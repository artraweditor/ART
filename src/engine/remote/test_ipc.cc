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

// Unit tests for the Unix socket transport and the wire protocol (no engine,
// no GUI): ctest -R ipc

#include "ipc_transport.h"
#include "wire.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

using namespace art::engine::remote;

namespace {

int failures = 0;

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            ++failures;                                                       \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: "    \
                      << #cond << std::endl;                                  \
        }                                                                     \
    } while (0)

std::string tmpdir;

std::string sock(const char *name) { return tmpdir + "/" + name; }

void test_roundtrip()
{
    std::string err;
    auto l = ipc::listen(sock("a.sock"), err);
    CHECK(l && err.empty());
    if (!l) {
        return;
    }
    struct stat st;
    CHECK(stat(sock("a.sock").c_str(), &st) == 0 && S_ISSOCK(st.st_mode));
    CHECK((st.st_mode & 0777) == 0600);

    wire::Request sent;
    sent.cwd = "/some dir";
    sent.mark = true;
    sent.args = {"open", "a file with spaces.arw", "--odd", "", "line\nbreak",
                 "caf\xc3\xa9"};

    // the client runs in a thread: connect, send, read the response
    std::string got_out, got_err;
    int got_status = -1;
    bool client_ok = false;
    std::thread client([&]() {
        std::string e;
        auto c = ipc::connect(sock("a.sock"), e);
        if (!c || !wire::writeRequest(*c, sent)) {
            return;
        }
        client_ok = wire::readResponse(
            *c,
            [&](char k, const std::string &d) {
                (k == 'O' ? got_out : got_err) += d;
            },
            got_status, e);
    });

    auto c = l->accept(5000);
    CHECK(c != nullptr);
    if (c) {
        wire::Request r;
        CHECK(wire::readRequest(*c, r, err));
        CHECK(r.cwd == sent.cwd && r.mark == sent.mark && r.args == sent.args);
        CHECK(wire::writeFrame(*c, 'O', "hello\n"));
        CHECK(wire::writeFrame(*c, 'E', "oops\n"));
        CHECK(wire::writeFrame(*c, 'O', "")); // nothing is sent
        CHECK(wire::writeFrame(*c, 'O', std::string("bin\0ary", 7)));
        CHECK(wire::writeStatus(*c, 3));
    }
    client.join();
    CHECK(client_ok);
    CHECK(got_out == std::string("hello\nbin\0ary", 13));
    CHECK(got_err == "oops\n");
    CHECK(got_status == 3);

    // no client: accept times out
    CHECK(l->accept(50) == nullptr);

    // a request with no command at all is valid
    std::thread client2([&]() {
        std::string e;
        auto cc = ipc::connect(sock("a.sock"), e);
        wire::Request empty;
        if (cc) {
            wire::writeRequest(*cc, empty);
        }
    });
    auto c2 = l->accept(5000);
    CHECK(c2 != nullptr);
    if (c2) {
        wire::Request r;
        CHECK(wire::readRequest(*c2, r, err) && r.args.empty() && !r.mark);
    }
    client2.join();

    l.reset();
    // the socket file goes away with the listener
    CHECK(stat(sock("a.sock").c_str(), &st) != 0);
}

void test_bad_requests()
{
    std::string err;
    auto l = ipc::listen(sock("b.sock"), err);
    CHECK(l != nullptr);
    if (!l) {
        return;
    }
    const struct {
        std::string bytes;
        const char *msg;
    } cases[] = {
        {"HELLO 3\nabc", "unsupported protocol"},
        {"ART2 3\nabc", "unsupported protocol"},
        {"ART1 x\n", "unsupported protocol"},
        {"ART1 99999999\n", "request too large"},
        {"ART1 5\nab", "incomplete request"}, // then the client hangs up
        {"ART1 3\nabc", "malformed request"}, // no NUL terminators
        {std::string("ART1 4\n/\0X\0", 11), "malformed request"}, // bad mark
    };
    for (auto &t : cases) {
        std::thread client([&]() {
            std::string e;
            auto c = ipc::connect(sock("b.sock"), e);
            if (c) {
                c->write(t.bytes.data(), t.bytes.size());
            }
        });
        auto c = l->accept(5000);
        CHECK(c != nullptr);
        if (c) {
            wire::Request r;
            std::string e;
            CHECK(!wire::readRequest(*c, r, e));
            CHECK(e == t.msg);
            if (e != t.msg) {
                std::cerr << "  for \"" << t.bytes << "\": " << e << std::endl;
            }
        }
        client.join();
    }
}

void test_bad_responses()
{
    std::string err;
    auto l = ipc::listen(sock("c.sock"), err);
    CHECK(l != nullptr);
    if (!l) {
        return;
    }
    const char *cases[] = {"Z 3\nabc", "O 3\nab", "O zz\n", "X\n", "O 3\nabc"};
    for (auto bytes : cases) {
        std::thread server([&]() {
            auto c = l->accept(5000);
            if (c) {
                c->write(bytes, strlen(bytes));
            } // and hangs up
        });
        std::string e;
        auto c = ipc::connect(sock("c.sock"), e);
        CHECK(c != nullptr);
        if (c) {
            int status = -1;
            // none of these ends with a status frame
            CHECK(!wire::readResponse(
                *c, [](char, const std::string &) {}, status, e));
            CHECK(status == -1 && !e.empty());
        }
        server.join();
    }
}

void test_listen_errors()
{
    std::string err;
    auto l = ipc::listen(sock("d.sock"), err);
    CHECK(l != nullptr);

    // a server is running there
    auto l2 = ipc::listen(sock("d.sock"), err);
    CHECK(l2 == nullptr);
    CHECK(err.find("already running") != std::string::npos);

    // a regular file is left alone
    FILE *f = fopen(sock("file").c_str(), "w");
    if (f) {
        fclose(f);
    }
    CHECK(ipc::listen(sock("file"), err) == nullptr);
    CHECK(err.find("not a socket") != std::string::npos);
    CHECK(access(sock("file").c_str(), F_OK) == 0);

    // a path that does not fit in sun_path
    CHECK(ipc::listen(tmpdir + "/" + std::string(300, 'x'), err) == nullptr);
    CHECK(err.find("too long") != std::string::npos);
    CHECK(ipc::connect(tmpdir + "/" + std::string(300, 'x'), err) == nullptr);
    CHECK(err.find("too long") != std::string::npos);

    // nothing there
    CHECK(ipc::connect(sock("nothing.sock"), err) == nullptr);
    CHECK(!err.empty());
}

void test_stale_socket()
{
    // a socket file that nobody listens on (a crashed server): replaced
    const std::string path = sock("stale.sock");
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path.c_str());
    CHECK(bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0);
    close(fd); // leaves the file behind
    struct stat st;
    CHECK(stat(path.c_str(), &st) == 0);

    std::string err;
    std::string e;
    CHECK(ipc::connect(path, e) == nullptr); // refused
    auto l = ipc::listen(path, err);
    CHECK(l != nullptr);
    if (l) {
        std::thread client([&]() {
            std::string e2;
            auto c = ipc::connect(path, e2);
            if (c) {
                wire::writeRequest(*c, wire::Request());
            }
        });
        CHECK(l->accept(5000) != nullptr);
        client.join();
    }
}

void test_default_path()
{
    std::string err;
    setenv("ART_SERVER_SOCKET", "/explicit/path.sock", 1);
    CHECK(ipc::defaultSocketPath(err) == "/explicit/path.sock");
    unsetenv("ART_SERVER_SOCKET");

    // the directory is created private, and re-used
    setenv("XDG_RUNTIME_DIR", tmpdir.c_str(), 1);
    std::string p = ipc::defaultSocketPath(err);
    CHECK(p == tmpdir + "/ART/server.sock");
    struct stat st;
    CHECK(stat((tmpdir + "/ART").c_str(), &st) == 0 &&
          (st.st_mode & 0777) == 0700);
    CHECK(ipc::defaultSocketPath(err) == p);

    // a too open directory of ours is made private
    chmod((tmpdir + "/ART").c_str(), 0755);
    CHECK(ipc::defaultSocketPath(err) == p);
    CHECK(stat((tmpdir + "/ART").c_str(), &st) == 0 &&
          (st.st_mode & 0777) == 0700);

    // a symlink in its place is refused
    unsetenv("XDG_RUNTIME_DIR");
    setenv("TMPDIR", tmpdir.c_str(), 1);
    setenv("USER", "someone", 1);
    CHECK(symlink("/tmp", (tmpdir + "/ART-someone").c_str()) == 0);
    CHECK(ipc::defaultSocketPath(err).empty() && !err.empty());
}

} // namespace

int main()
{
    char tmpl[] = "/tmp/art-ipc-test-XXXXXX";
    if (!mkdtemp(tmpl)) {
        std::cerr << "mkdtemp failed" << std::endl;
        return 2;
    }
    tmpdir = tmpl;

    test_roundtrip();
    test_bad_requests();
    test_bad_responses();
    test_listen_errors();
    test_stale_socket();
    test_default_path();

    // clean up what is left in the test directory
    const std::string cmd = "rm -rf '" + tmpdir + "'";
    if (system(cmd.c_str()) != 0) {
        std::cerr << "could not remove " << tmpdir << std::endl;
    }
    if (failures) {
        std::cerr << failures << " check(s) failed" << std::endl;
        return 1;
    }
    std::cout << "ipc: all checks passed" << std::endl;
    return 0;
}
