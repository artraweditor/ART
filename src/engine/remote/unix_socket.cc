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

#else // _WIN32: AF_UNIX sockets of Winsock (Windows 10 1803 and later)

#include <winsock2.h>
#include <afunix.h>
#include <windows.h>
#include <cstdlib>
#include <cstring>
#include <vector>

#ifndef SIO_AF_UNIX_GETPEERPID
#define SIO_AF_UNIX_GETPEERPID _WSAIOR(IOC_VENDOR, 256)
#endif

namespace art { namespace engine { namespace remote { namespace ipc {

namespace {

constexpr DWORD REQUEST_TIMEOUT_MS = 10000;

std::string winerr(const std::string &what, int e)
{
    char buf[256];
    DWORD n = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
        DWORD(e), 0, buf, sizeof(buf), nullptr);
    std::string msg;
    if (n == 0) {
        msg = "error " + std::to_string(e);
    } else {
        while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' ||
                         buf[n - 1] == ' ' || buf[n - 1] == '.')) {
            --n;
        }
        msg.assign(buf, n);
    }
    return what + ": " + msg;
}


bool init_winsock()
{
    static const bool ok = []() {
        WSADATA d;
        return WSAStartup(MAKEWORD(2, 2), &d) == 0;
    }();
    return ok;
}


void no_inherit(SOCKET s)
{
    SetHandleInformation(reinterpret_cast<HANDLE>(s), HANDLE_FLAG_INHERIT, 0);
}


class SockConnection: public Connection {
public:
    explicit SockConnection(SOCKET s): s_(s) { no_inherit(s_); }
    ~SockConnection() override { ::closesocket(s_); }

    void setReceiveTimeout(DWORD ms)
    {
        setsockopt(s_, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char *>(&ms), sizeof(ms));
    }

    bool read(void *buf, size_t n) override
    {
        char *p = static_cast<char *>(buf);
        while (n > 0) {
            int r = ::recv(s_, p, int(n > 0x100000 ? 0x100000 : n), 0);
            if (r <= 0) {
                return false; // end of stream, error or timeout
            }
            p += r;
            n -= size_t(r);
        }
        return true;
    }

    bool write(const void *buf, size_t n) override
    {
        const char *p = static_cast<const char *>(buf);
        while (n > 0) {
            int r = ::send(s_, p, int(n > 0x100000 ? 0x100000 : n), 0);
            if (r <= 0) {
                return false;
            }
            p += r;
            n -= size_t(r);
        }
        return true;
    }

private:
    SOCKET s_;
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


// connected socket, or INVALID_SOCKET with WSAGetLastError() set (and `err`
// set if the address is bad)
SOCKET connect_sock(const std::string &path, std::string &err, bool &bad_addr)
{
    sockaddr_un addr;
    bad_addr = !fill_addr(path, addr, err);
    if (bad_addr) {
        return INVALID_SOCKET;
    }
    SOCKET s = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }
    if (::connect(s, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        int e = WSAGetLastError();
        ::closesocket(s);
        WSASetLastError(e);
        return INVALID_SOCKET;
    }
    return s;
}


// the SID of the user of a process (an open token); empty on failure
std::vector<char> token_user(HANDLE token)
{
    DWORD n = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &n);
    std::vector<char> v(n);
    if (n == 0 || !GetTokenInformation(token, TokenUser, v.data(), n, &n)) {
        v.clear();
    }
    return v;
}


bool same_user(const std::vector<char> &a, const std::vector<char> &b)
{
    return !a.empty() && !b.empty() &&
           EqualSid(reinterpret_cast<const TOKEN_USER *>(a.data())->User.Sid,
                    reinterpret_cast<const TOKEN_USER *>(b.data())->User.Sid);
}


// Is the client the same user as we are? Where Windows tells us the pid of the
// peer, its user is compared with ours; otherwise (old Windows 10) the access
// rules of the socket's directory are all that protects the server.
bool peer_is_self(SOCKET s)
{
    ULONG pid = 0;
    DWORD got = 0;
    if (WSAIoctl(s, SIO_AF_UNIX_GETPEERPID, nullptr, 0, &pid, sizeof(pid),
                 &got, nullptr, nullptr) != 0) {
        return true;
    }
    HANDLE me = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &me)) {
        return false;
    }
    std::vector<char> mine = token_user(me);
    CloseHandle(me);

    HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!proc) {
        return false;
    }
    HANDLE tok = nullptr;
    bool ok = false;
    if (OpenProcessToken(proc, TOKEN_QUERY, &tok)) {
        ok = same_user(mine, token_user(tok));
        CloseHandle(tok);
    }
    CloseHandle(proc);
    return ok;
}


class SockListener: public Listener {
public:
    SockListener(SOCKET s, const std::string &path): s_(s), path_(path)
    {
        no_inherit(s_);
    }

    ~SockListener() override
    {
        ::closesocket(s_);
        ::DeleteFileA(path_.c_str());
    }

    std::unique_ptr<Connection> accept(int timeout_ms) override
    {
        WSAPOLLFD p;
        p.fd = s_;
        p.events = POLLRDNORM;
        p.revents = 0;
        if (::WSAPoll(&p, 1, timeout_ms) <= 0 || !(p.revents & POLLRDNORM)) {
            return nullptr;
        }
        SOCKET c = ::accept(s_, nullptr, nullptr);
        if (c == INVALID_SOCKET) {
            return nullptr;
        }
        if (!peer_is_self(c)) {
            ::closesocket(c);
            return nullptr;
        }
        auto ret = new SockConnection(c);
        ret->setReceiveTimeout(REQUEST_TIMEOUT_MS);
        return std::unique_ptr<Connection>(ret);
    }

private:
    SOCKET s_;
    std::string path_;
};


// make sure `dir` is a real directory (not a link) that exists; it lives under
// the user's own temporary directory, whose access rules keep others out
bool private_dir(const std::string &dir, std::string &err)
{
    if (!CreateDirectoryA(dir.c_str(), nullptr) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        err = winerr("cannot create " + dir, int(GetLastError()));
        return false;
    }
    DWORD a = GetFileAttributesA(dir.c_str());
    if (a == INVALID_FILE_ATTRIBUTES) {
        err = winerr("cannot examine " + dir, int(GetLastError()));
        return false;
    }
    if (!(a & FILE_ATTRIBUTE_DIRECTORY) || (a & FILE_ATTRIBUTE_REPARSE_POINT)) {
        err = dir + " is not a directory";
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
    const char *tmp = getenv("TEMP");
    if (!tmp || !*tmp) {
        tmp = getenv("TMP");
    }
    if (!tmp || !*tmp) {
        err = "no temporary directory (TEMP) for the socket; use --socket or "
              "ART_SERVER_SOCKET";
        return "";
    }
    std::string t = tmp;
    while (t.size() > 1 && (t.back() == '\\' || t.back() == '/')) {
        t.pop_back();
    }
    const char *user = getenv("USERNAME");
    // the same layout as the client of libART (art_remote.c)
    std::string dir = t + "\\ART-" + ((user && *user) ? user : "user");
    if (!private_dir(dir, err)) {
        return "";
    }
    return dir + "\\server.sock";
}


std::unique_ptr<Connection> connect(const std::string &path, std::string &err)
{
    if (!init_winsock()) {
        err = "cannot initialize Winsock";
        return nullptr;
    }
    bool bad = false;
    SOCKET s = connect_sock(path, err, bad);
    if (s == INVALID_SOCKET) {
        if (!bad) {
            err = winerr(path, WSAGetLastError());
        }
        return nullptr;
    }
    return std::unique_ptr<Connection>(new SockConnection(s));
}


std::unique_ptr<Listener> listen(const std::string &path, std::string &err)
{
    if (!init_winsock()) {
        err = "cannot initialize Winsock";
        return nullptr;
    }
    sockaddr_un addr;
    if (!fill_addr(path, addr, err)) {
        return nullptr;
    }

    const DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        // a socket file is a reparse point
        if ((attrs & FILE_ATTRIBUTE_DIRECTORY) ||
            !(attrs & FILE_ATTRIBUTE_REPARSE_POINT)) {
            err = path + " exists and is not a socket";
            return nullptr;
        }
        std::string e;
        bool bad;
        SOCKET s = connect_sock(path, e, bad);
        if (s != INVALID_SOCKET) {
            ::closesocket(s);
            err = "a server is already running on " + path;
            return nullptr;
        }
        const int we = WSAGetLastError();
        if (we != WSAECONNREFUSED && we != WSAEINVAL) {
            err = winerr("cannot use " + path, we);
            return nullptr;
        }
        // nobody is listening: left over by a server that did not exit cleanly
        if (!DeleteFileA(path.c_str())) {
            err = winerr("cannot remove " + path, int(GetLastError()));
            return nullptr;
        }
    }

    SOCKET s = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        err = winerr("socket", WSAGetLastError());
        return nullptr;
    }
    if (::bind(s, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        err = winerr("cannot bind " + path, WSAGetLastError());
        ::closesocket(s);
        return nullptr;
    }
    if (::listen(s, 16) != 0) {
        err = winerr("cannot listen on " + path, WSAGetLastError());
        ::closesocket(s);
        ::DeleteFileA(path.c_str());
        return nullptr;
    }
    return std::unique_ptr<Listener>(new SockListener(s, path));
}

}}}} // namespace art::engine::remote::ipc

#endif
