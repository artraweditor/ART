/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  ART is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ART is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ART.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ipc_transport.h"

#ifndef _WIN32

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

namespace art { namespace engine { namespace remote { namespace ipc {

namespace {

constexpr int REQUEST_TIMEOUT_S = 10;

std::string errstr(const std::string &what, int e)
{
    return what + ": " + strerror(e);
}


void set_cloexec(int fd)
{
    int fl = fcntl(fd, F_GETFD);
    if (fl >= 0) {
        fcntl(fd, F_SETFD, fl | FD_CLOEXEC);
    }
}


class FdConnection: public Connection {
public:
    explicit FdConnection(int fd): fd_(fd)
    {
        set_cloexec(fd_);
#ifdef SO_NOSIGPIPE
        int one = 1;
        setsockopt(fd_, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
    }
    ~FdConnection() override { ::close(fd_); }

    void setReceiveTimeout(int seconds)
    {
        timeval tv;
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    bool read(void *buf, size_t n) override
    {
        char *p = static_cast<char *>(buf);
        while (n > 0) {
            ssize_t r = ::read(fd_, p, n);
            if (r < 0 && errno == EINTR) {
                continue;
            }
            if (r <= 0) {
                return false;
            }
            p += r;
            n -= size_t(r);
        }
        return true;
    }

    bool write(const void *buf, size_t n) override
    {
        const char *p = static_cast<const char *>(buf);
#ifdef MSG_NOSIGNAL
        const int flags = MSG_NOSIGNAL;
#else
        const int flags = 0;
#endif
        while (n > 0) {
            ssize_t r = ::send(fd_, p, n, flags);
            if (r < 0 && errno == EINTR) {
                continue;
            }
            if (r <= 0) {
                return false;
            }
            p += r;
            n -= size_t(r);
        }
        return true;
    }

private:
    int fd_;
};


bool fill_addr(const std::string &path, sockaddr_un &addr, std::string &err)
{
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (path.empty() || path.size() >= sizeof(addr.sun_path)) {
        err = "socket path too long (at most " +
              std::to_string(sizeof(addr.sun_path) - 1) +
              " bytes; use --socket or ART_SERVER_SOCKET to choose another): " +
              path;
        return false;
    }
    memcpy(addr.sun_path, path.c_str(), path.size());
    return true;
}


// connected fd, or -1 with errno set (and `err` set if the address is bad)
int connect_fd(const std::string &path, std::string &err, bool &bad_addr)
{
    sockaddr_un addr;
    bad_addr = !fill_addr(path, addr, err);
    if (bad_addr) {
        return -1;
    }
    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        int e = errno;
        ::close(fd);
        errno = e;
        return -1;
    }
    return fd;
}


bool peer_is_self(int fd)
{
#if defined(__linux__)
    ucred cr;
    socklen_t len = sizeof(cr);
    return getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cr, &len) == 0 &&
           cr.uid == geteuid();
#else
    uid_t u;
    gid_t g;
    return getpeereid(fd, &u, &g) == 0 && u == geteuid();
#endif
}


class UnixListener: public Listener {
public:
    UnixListener(int fd, const std::string &path): fd_(fd), path_(path)
    {
        set_cloexec(fd_);
    }

    ~UnixListener() override
    {
        ::close(fd_);
        ::unlink(path_.c_str());
    }

    std::unique_ptr<Connection> accept(int timeout_ms) override
    {
        pollfd p;
        p.fd = fd_;
        p.events = POLLIN;
        p.revents = 0;
        if (::poll(&p, 1, timeout_ms) <= 0 || !(p.revents & POLLIN)) {
            return nullptr;
        }
        int c = ::accept(fd_, nullptr, nullptr);
        if (c < 0) {
            return nullptr;
        }
        if (!peer_is_self(c)) {
            ::close(c);
            return nullptr;
        }
        auto ret = new FdConnection(c);
        ret->setReceiveTimeout(REQUEST_TIMEOUT_S);
        return std::unique_ptr<Connection>(ret);
    }

private:
    int fd_;
    std::string path_;
};


// make sure `dir` is a directory of ours that nobody else can enter
bool private_dir(const std::string &dir, std::string &err)
{
    if (::mkdir(dir.c_str(), 0700) != 0 && errno != EEXIST) {
        err = errstr("cannot create " + dir, errno);
        return false;
    }
    struct stat st;
    if (::lstat(dir.c_str(), &st) != 0) {
        err = errstr("cannot examine " + dir, errno);
        return false;
    }
    if (!S_ISDIR(st.st_mode)) { // also refuses a symlink
        err = dir + " is not a directory";
        return false;
    }
    if (st.st_uid != geteuid()) {
        err = dir + " belongs to another user";
        return false;
    }
    if ((st.st_mode & 077) != 0 && ::chmod(dir.c_str(), 0700) != 0) {
        err = errstr("cannot make " + dir + " private", errno);
        return false;
    }
    return true;
}

} // namespace


std::string defaultSocketPath(std::string &err)
{
    const char *env = getenv("ART_SERVER_SOCKET");
    if (env && *env) {
        return env;
    }
    std::string dir;
    const char *xdg = getenv("XDG_RUNTIME_DIR");
    if (xdg && xdg[0] == '/') {
        dir = std::string(xdg) + "/ART";
    } else {
        const char *tmp = getenv("TMPDIR");
        std::string t = (tmp && *tmp) ? tmp : "/tmp";
        while (t.size() > 1 && t.back() == '/') {
            t.pop_back();
        }
        const char *user = getenv("USER");
        dir = t + "/ART-" +
              ((user && *user) ? std::string(user)
                               : std::to_string(unsigned(geteuid())));
    }
    if (!private_dir(dir, err)) {
        return "";
    }
    return dir + "/server.sock";
}


std::unique_ptr<Connection> connect(const std::string &path, std::string &err)
{
    bool bad = false;
    int fd = connect_fd(path, err, bad);
    if (fd < 0) {
        if (!bad) {
            err = errstr(path, errno);
        }
        return nullptr;
    }
    return std::unique_ptr<Connection>(new FdConnection(fd));
}


std::unique_ptr<Listener> listen(const std::string &path, std::string &err)
{
    sockaddr_un addr;
    if (!fill_addr(path, addr, err)) {
        return nullptr;
    }

    struct stat st;
    if (::lstat(path.c_str(), &st) == 0) {
        if (!S_ISSOCK(st.st_mode)) {
            err = path + " exists and is not a socket";
            return nullptr;
        }
        std::string e;
        bool bad;
        int fd = connect_fd(path, e, bad);
        if (fd >= 0) {
            ::close(fd);
            err = "a server is already running on " + path;
            return nullptr;
        }
        if (errno != ECONNREFUSED && errno != ENOENT) {
            err = errstr("cannot use " + path, errno);
            return nullptr;
        }
        // nobody is listening: left over by a server that did not exit cleanly
        if (st.st_uid != geteuid()) {
            err = path + " belongs to another user";
            return nullptr;
        }
        ::unlink(path.c_str());
    }

    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        err = errstr("socket", errno);
        return nullptr;
    }
    const mode_t old = ::umask(077);
    const int rc = ::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    const int bind_errno = errno;
    ::umask(old);
    if (rc != 0) {
        ::close(fd);
        err = errstr("cannot bind " + path, bind_errno);
        return nullptr;
    }
    ::chmod(path.c_str(), 0600); // in case umask was not honoured
    if (::listen(fd, 16) != 0) {
        err = errstr("cannot listen on " + path, errno);
        ::close(fd);
        ::unlink(path.c_str());
        return nullptr;
    }
    return std::unique_ptr<Listener>(new UnixListener(fd, path));
}

}}}} // namespace art::engine::remote::ipc

#else // _WIN32: no transport yet

namespace art { namespace engine { namespace remote { namespace ipc {

static const char *UNSUPPORTED =
    "the server mode is not supported on this platform yet";

std::string defaultSocketPath(std::string &err)
{
    err = UNSUPPORTED;
    return "";
}

std::unique_ptr<Connection> connect(const std::string &, std::string &err)
{
    err = UNSUPPORTED;
    return nullptr;
}

std::unique_ptr<Listener> listen(const std::string &, std::string &err)
{
    err = UNSUPPORTED;
    return nullptr;
}

}}}} // namespace art::engine::remote::ipc

#endif
