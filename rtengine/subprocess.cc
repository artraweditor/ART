/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2020 Alberto Griggio <alberto.griggio@gmail.com>
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

#ifdef WIN32
// We need InitializeProcThreadAttributeList/PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
// which require Windows Vista or later. This has to come before any header that
// might pull in <windows.h>.
#undef WINVER
#define WINVER 0x0600
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <giomm.h>
#include <glib/gstdio.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include <cstdlib>
#include <set>
#include <vector>

#ifdef WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include "../rtgui/pathutils.h"
#include "settings.h"
#include "subprocess.h"

namespace rtengine {

extern const Settings *settings;

namespace subprocess {

std::wstring to_wstr(const Glib::ustring &s)
{
    auto *ws = g_utf8_to_utf16(s.c_str(), -1, nullptr, nullptr, nullptr);
    std::wstring ret(reinterpret_cast<wchar_t *>(ws));
    g_free(ws);
    return ret;
}

#ifdef WIN32

std::vector<Glib::ustring> split_command_line(const Glib::ustring &cmdl)
{
    std::wstring w = to_wstr(cmdl);
    int n;
    LPWSTR *a = CommandLineToArgvW(w.c_str(), &n);
    if (!a) {
        throw(error() << "impossible to split command line: " << cmdl);
    }

    std::vector<Glib::ustring> ret;
    for (int i = 0; i < n; ++i) {
        auto *s = g_utf16_to_utf8(reinterpret_cast<gunichar2 *>(a[i]), -1,
                                  nullptr, nullptr, nullptr);
        if (!s) {
            throw(error() << "impossible to split command line: " << cmdl);
        }
        ret.push_back(s);
        g_free(s);
    }
    LocalFree(a);

    return ret;
}

namespace {

void add_quoted(std::wostream &out, const std::wstring &ws)
{
    for (size_t j = 0; j < ws.size();) {
        int backslashes = 0;
        while (j < ws.size() && ws[j] == '\\') {
            ++backslashes;
            ++j;
        }
        if (j == ws.size()) {
            backslashes = backslashes * 2;
        } else if (ws[j] == '"') {
            backslashes = backslashes * 2 + 1;
        }
        for (int i = 0; i < backslashes; ++i) {
            out << '\\';
        }
        if (j < ws.size()) {
            if (isspace(ws[j])) {
                out << '"' << ws[j] << '"';
            } else {
                out << ws[j];
            }
            ++j;
        } else {
            break;
        }
    }
}

struct HandleCloser {
    ~HandleCloser()
    {
        for (auto h : toclose) {
            CloseHandle(h);
        }
    }
    std::set<HANDLE> toclose;
};

// Restricts what a child process inherits to the handles it actually needs.
//
// Passing bInheritHandles = TRUE to CreateProcessW without a handle list gives
// the child a copy of *every* inheritable handle in the process, including the
// ones the CRT creates for the image files we have open. For a long-lived child
// such as the exiftool "-stay_open" worker, that keeps those files locked for
// the whole session, even after ART itself has closed them -- which is the
// "must quit ART to delete the file" part of issue #398.
class InheritList {
public:
    InheritList(): attrs_(nullptr) {}

    ~InheritList()
    {
        if (attrs_) {
            DeleteProcThreadAttributeList(
                static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrs_));
            free(attrs_);
        }
    }

    InheritList(const InheritList &) = delete;
    InheritList &operator=(const InheritList &) = delete;

    // Registers a handle the child needs. Handles that cannot be made
    // inheritable are silently dropped -- the child would not have got a usable
    // copy of them anyway.
    void add(HANDLE h)
    {
        if (h == nullptr || h == INVALID_HANDLE_VALUE) {
            return;
        }
        DWORD flags = 0;
        if (!GetHandleInformation(h, &flags)) {
            return;
        }
        if (!(flags & HANDLE_FLAG_INHERIT) &&
            !SetHandleInformation(h, HANDLE_FLAG_INHERIT,
                                  HANDLE_FLAG_INHERIT)) {
            return;
        }
        for (auto x : handles_) {
            if (x == h) {
                return; // already there (e.g. stdout == stderr)
            }
        }
        handles_.push_back(h);
    }

    // Sets up si for CreateProcessW. Returns true if the child should be
    // created with bInheritHandles = TRUE and EXTENDED_STARTUPINFO_PRESENT,
    // false if it should inherit nothing at all.
    bool build(STARTUPINFOEXW &si)
    {
        if (handles_.empty()) {
            return false;
        }

        SIZE_T size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        attrs_ = malloc(size);
        if (!attrs_) {
            return false;
        }
        auto *lst = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrs_);
        if (!InitializeProcThreadAttributeList(lst, 1, 0, &size)) {
            free(attrs_);
            attrs_ = nullptr;
            return false;
        }
        if (!UpdateProcThreadAttribute(lst, 0,
                                       PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                       handles_.data(),
                                       handles_.size() * sizeof(HANDLE),
                                       nullptr, nullptr)) {
            DeleteProcThreadAttributeList(lst);
            free(attrs_);
            attrs_ = nullptr;
            return false;
        }

        si.lpAttributeList = lst;
        si.StartupInfo.cb = sizeof(STARTUPINFOEXW);
        return true;
    }

private:
    std::vector<HANDLE> handles_;
    void *attrs_;
};

} // namespace

// Glib::spawn_sync opens a console window for command-line apps, I wasn't
// able to find out how not to do that (see also:
// http://gtk.10911.n7.nabble.com/g-spawn-on-windows-td84743.html).
// Therefore, we roll our own
void exec_sync(const Glib::ustring &workdir,
               const std::vector<Glib::ustring> &argv, bool search_in_path,
               std::string *out, std::string *err)
{
    // TODO - capturing stdout/stderr leads to ReadFile hanging sometimes on
    // windows. I still have to figure out why though. In the meantime, we
    // simply don't support it (currently in ART out and err are used only for
    // informative purposes in verbose console output)
    out = nullptr;
    err = nullptr;

    HANDLE fds_from[2];
    HANDLE fds_from_e[2];
    SECURITY_ATTRIBUTES sa;
    HandleCloser hc;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    const auto mkpipe = [&](HANDLE *fd) -> bool {
        if (!CreatePipe(&(fd[0]), &(fd[1]), &sa, 0)) {
            return false;
        }
        hc.toclose.insert(fd[0]);
        hc.toclose.insert(fd[1]);
        if (!SetHandleInformation(fd[0], HANDLE_FLAG_INHERIT, 0)) {
            return false;
        }
        return true;
    };

    if (out || err) {
        if (!mkpipe(fds_from) || !mkpipe(fds_from_e)) {
            throw(error() << "mkpipe failed");
        }
    }

    PROCESS_INFORMATION pi;
    STARTUPINFOEXW six;
    InheritList inherit;

    ZeroMemory(&six, sizeof(STARTUPINFOEXW));
    STARTUPINFOW &si = six.StartupInfo;
    si.cb = sizeof(STARTUPINFOW);
    si.wShowWindow = SW_HIDE;
    if (out || err) {
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = fds_from[1];
        si.hStdError = fds_from_e[1];
        inherit.add(si.hStdInput);
        inherit.add(si.hStdOutput);
        inherit.add(si.hStdError);
    }

    std::wstring pth = to_wstr(argv[0]);
    if (search_in_path) {
        wchar_t pathbuf[MAX_PATH + 1];
        std::wstring suffix = to_wstr(".exe");
        int n = SearchPathW(nullptr, pth.c_str(), suffix.c_str(), MAX_PATH + 1,
                            pathbuf, nullptr);
        if (n > 0) {
            pth = pathbuf;
        }
    }

    wchar_t *cmdline = nullptr;
    {
        std::wostringstream cmdlinebuf;
        add_quoted(cmdlinebuf, pth);
        for (size_t i = 1; i < argv.size(); ++i) {
            cmdlinebuf << ' ';
            add_quoted(cmdlinebuf, to_wstr(argv[i]));
        }
        std::wstring s = cmdlinebuf.str();
        cmdline = new wchar_t[s.size() + 1];
        memcpy(cmdline, s.c_str(), s.size() * sizeof(wchar_t));
        cmdline[s.size()] = 0;
    }

    // only let the child inherit the handles it actually needs (issue #398)
    DWORD creation_flags = CREATE_NO_WINDOW;
    BOOL inherit_handles = FALSE;
    if (inherit.build(six)) {
        creation_flags |= EXTENDED_STARTUPINFO_PRESENT;
        inherit_handles = TRUE;
    }

    std::wstring wd = to_wstr(workdir);
    if (!CreateProcessW(pth.c_str(), cmdline, nullptr, nullptr, inherit_handles,
                        creation_flags, (LPVOID) nullptr,
                        wd.empty() ? nullptr : wd.c_str(), &si, &pi)) {
        delete[] cmdline;
        throw(error() << "impossible to create process");
    } else {
        hc.toclose.insert(pi.hProcess);
        hc.toclose.insert(pi.hThread);
    }
    delete[] cmdline;

    const auto read_pipe = [&](HANDLE *fd) -> std::string {
        constexpr size_t bufsize = 4096;
        unsigned char buf[bufsize];
        std::ostringstream sbuf;
        DWORD n;

        hc.toclose.erase(fd[1]);
        CloseHandle(fd[1]);

        while (ReadFile(fd[0], buf, bufsize, &n, nullptr)) {
            buf[n] = 0;
            sbuf << buf;
            if (n < bufsize) {
                break;
            }
        }
        return sbuf.str();
    };

    if (out || err) {
        std::string o = read_pipe(fds_from);
        if (out) {
            *out = std::move(o);
        }
        std::string e = read_pipe(fds_from_e);
        if (err) {
            *err = std::move(e);
        }
    }

    unsigned int status = 255;
    const DWORD wait_timeout_ms = INFINITE;
    if (WaitForSingleObject(pi.hProcess, wait_timeout_ms) != WAIT_OBJECT_0) {
        TerminateProcess(pi.hProcess, status);
    }
    if (!GetExitCodeProcess(pi.hProcess, (LPDWORD)&status)) {
        status = 255;
    }

    if (status != 0) {
        throw(error() << "exit status: " << status);
    }
}

std::wstring quote(const std::wstring &s)
{
    std::wostringstream out;
    add_quoted(out, s);
    return out.str();
}

class SubprocessData {
public:
    SubprocessData() = default;
    ~SubprocessData()
    {
        for (auto h : toclose) {
            CloseHandle(h);
        }
    }

    std::set<HANDLE> toclose;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    STARTUPINFOEXW six;
    HANDLE child_in;
    HANDLE child_out;
};

SubprocessData *D(uintptr_t impl)
{
    return reinterpret_cast<SubprocessData *>(impl);
}

SubprocessInfo::~SubprocessInfo()
{
    kill();
    delete D(impl_);
}

bool SubprocessInfo::live() const
{
    DWORD exitcode = 0;
    return GetExitCodeProcess(D(impl_)->pi.hProcess, &exitcode) &&
           exitcode == STILL_ACTIVE;
}

int SubprocessInfo::wait()
{
    const DWORD wait_timeout_ms = INFINITE;
    if (WaitForSingleObject(D(impl_)->pi.hProcess, wait_timeout_ms) !=
        WAIT_OBJECT_0) {
        kill();
        return -1;
    }
    DWORD status = -1;
    GetExitCodeProcess(D(impl_)->pi.hProcess, &status);
    return status;
}

void SubprocessInfo::kill() { TerminateProcess(D(impl_)->pi.hProcess, 255); }

int SubprocessInfo::read()
{
    char buf[1] = {0};
    if (!ReadFile(D(impl_)->child_out, &buf, 1, nullptr, nullptr)) {
        return EOF;
    }
    return buf[0];
}

bool SubprocessInfo::write(const char *msg, size_t n)
{
    DWORD w = 0;
    return WriteFile(D(impl_)->child_in, msg, n, &w, nullptr);
}

bool SubprocessInfo::flush() { return FlushFileBuffers(D(impl_)->child_in); }

int SubprocessInfo::id() const { return GetProcessId(D(impl_)->pi.hProcess); }

std::unique_ptr<SubprocessInfo> popen(const Glib::ustring &workdir,
                                      const std::vector<Glib::ustring> &argv,
                                      bool search_in_path, bool pipe_in,
                                      bool pipe_out)
{
    std::unique_ptr<SubprocessData> data(new SubprocessData());

    HANDLE fds_to[2];
    HANDLE fds_from[2];
    SECURITY_ATTRIBUTES &sa = data->sa;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    const auto mkpipe = [&](HANDLE *fd, int direction) -> bool {
        if (!CreatePipe(&(fd[0]), &(fd[1]), &sa, 0)) {
            return false;
        }
        data->toclose.insert(fd[0]);
        data->toclose.insert(fd[1]);
        if (!SetHandleInformation(fd[direction], HANDLE_FLAG_INHERIT, 0)) {
            return false;
        }
        return true;
    };

    if (pipe_in && !mkpipe(fds_to, 1)) {
        throw(error() << "mkpipe failed");
    }
    if (pipe_out && !mkpipe(fds_from, 0)) {
        throw(error() << "mkpipe failed");
    }

    PROCESS_INFORMATION &pi = data->pi;
    STARTUPINFOEXW &six = data->six;
    STARTUPINFOW &si = six.StartupInfo;
    InheritList inherit;

    ZeroMemory(&six, sizeof(STARTUPINFOEXW));
    si.cb = sizeof(STARTUPINFOW);
    si.wShowWindow = SW_HIDE;
    si.dwFlags = STARTF_USESTDHANDLES;

    if (pipe_in) {
        si.hStdInput = fds_to[0];
    } else {
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }
    if (pipe_out) {
        si.hStdOutput = fds_from[1];
        si.hStdError = fds_from[1];
    } else {
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }

    inherit.add(si.hStdInput);
    inherit.add(si.hStdOutput);
    inherit.add(si.hStdError);

    std::wstring pth = to_wstr(argv[0]);
    if (search_in_path) {
        wchar_t pathbuf[MAX_PATH + 1];
        std::wstring suffix = to_wstr(".exe");
        int n = SearchPathW(nullptr, pth.c_str(), suffix.c_str(), MAX_PATH + 1,
                            pathbuf, nullptr);
        if (n > 0) {
            pth = pathbuf;
        }
    }

    wchar_t *cmdline = nullptr;
    {
        std::wostringstream cmdlinebuf;
        add_quoted(cmdlinebuf, pth);
        for (size_t i = 1; i < argv.size(); ++i) {
            cmdlinebuf << ' ';
            add_quoted(cmdlinebuf, to_wstr(argv[i]));
        }
        std::wstring s = cmdlinebuf.str();
        cmdline = new wchar_t[s.size() + 1];
        memcpy(cmdline, s.c_str(), s.size() * sizeof(wchar_t));
        cmdline[s.size()] = 0;
    }

    // only let the child inherit the handles it actually needs (issue #398)
    DWORD creation_flags = CREATE_NO_WINDOW;
    BOOL inherit_handles = FALSE;
    if (inherit.build(six)) {
        creation_flags |= EXTENDED_STARTUPINFO_PRESENT;
        inherit_handles = TRUE;
    }

    std::wstring wd = to_wstr(workdir);
    if (!CreateProcessW(pth.c_str(), cmdline, nullptr, nullptr, inherit_handles,
                        creation_flags, (LPVOID) nullptr,
                        wd.empty() ? nullptr : wd.c_str(), &si, &pi)) {
        delete[] cmdline;
        throw(error() << "impossible to create process");
    } else {
        data->toclose.insert(pi.hProcess);
        data->toclose.insert(pi.hThread);
    }
    delete[] cmdline;

    if (pipe_in) {
        data->toclose.erase(fds_to[1]);
    }

    if (pipe_out) {
        data->toclose.erase(fds_from[0]);
    }

    auto impl = data.release();
    impl->child_out = fds_from[0];
    impl->child_in = fds_to[1];
    std::unique_ptr<SubprocessInfo> res(
        new SubprocessInfo(reinterpret_cast<uintptr_t>(impl)));

    return res;
}

#else // WIN32

std::vector<Glib::ustring> split_command_line(const Glib::ustring &cmdl)
{
    try {
        auto argv = Glib::shell_parse_argv(cmdl);
        std::vector<Glib::ustring> ret;
        for (const auto &a : argv) {
            ret.push_back(fname_to_utf8(a));
        }
        return ret;
    } catch (Glib::Error &e) {
        throw(error() << e.what());
    }
}

void exec_sync(const Glib::ustring &workdir,
               const std::vector<Glib::ustring> &argv, bool search_in_path,
               std::string *out, std::string *err)
{
    std::vector<std::string> args;
    args.reserve(argv.size());
    for (auto &s : argv) {
        args.push_back(Glib::filename_from_utf8(s));
    }
    try {
        int exit_status = -1;
        auto flags = Glib::SPAWN_DEFAULT;
        if (search_in_path) {
            flags |= Glib::SPAWN_SEARCH_PATH;
        }
        std::string wd = Glib::filename_from_utf8(workdir);
        Glib::spawn_sync(wd, args, get_env(), flags,
                         Glib::SlotSpawnChildSetup(), out, err, &exit_status);
        if (!(WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == 0)) {
            throw(error() << "exit status: " << exit_status);
        }
    } catch (Glib::Exception &e) {
        throw(error() << e.what());
    }
}

class SubprocessData {
public:
    SubprocessData() = default;
    ~SubprocessData()
    {
        for (auto d : toclose) {
            close(d);
        }
    }
    std::set<int> toclose;
    int child_in;
    int child_out;
    pid_t pid;
};

SubprocessData *D(uintptr_t impl)
{
    return reinterpret_cast<SubprocessData *>(impl);
}

SubprocessInfo::~SubprocessInfo() { delete D(impl_); }

bool SubprocessInfo::live() const
{
    int status = 0;
    auto pid = D(impl_)->pid;
    if (pid < 0 || waitpid(pid, &status, WNOHANG)) {
        return false;
    } else {
        return true;
    }
}

int SubprocessInfo::wait()
{
    int status = 0;
    auto pid = D(impl_)->pid;
    if (pid > 0) {
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    } else {
        return -1;
    }
}

void SubprocessInfo::kill()
{
    if (live()) {
        ::kill(D(impl_)->pid, SIGTERM);
    }
}

int SubprocessInfo::read()
{
    char buf[1] = {0};
    if (::read(D(impl_)->child_out, buf, 1) <= 0) {
        return EOF;
    }
    return buf[0];
}

bool SubprocessInfo::write(const char *msg, size_t n)
{
    return ::write(D(impl_)->child_in, msg, n) >= 0;
}

bool SubprocessInfo::flush()
{
    fsync(D(impl_)->child_in);
    return true;
}

std::unique_ptr<SubprocessInfo> popen(const Glib::ustring &workdir,
                                      const std::vector<Glib::ustring> &argv,
                                      bool search_in_path, bool pipe_in,
                                      bool pipe_out)
{
    int fds_to[2];
    int fds_from[2];

    std::unique_ptr<SubprocessInfo> res;
    std::unique_ptr<SubprocessData> data(new SubprocessData());

    std::string executable = "";
    if (argv.empty()) {
        throw(error() << "invalid arguments");
    } else {
        if (!search_in_path) {
            executable = argv[0];
        } else {
            executable = Glib::find_program_in_path(argv[0]);
            if (!Glib::file_test(executable, Glib::FILE_TEST_IS_EXECUTABLE) &&
                argv[0].find('/') == Glib::ustring::npos && !workdir.empty()) {
                executable = Glib::build_filename(workdir, argv[0]);
            }
        }
        if (!Glib::file_test(executable, Glib::FILE_TEST_IS_EXECUTABLE)) {
            throw(error() << "executable not found: " << argv[0]);
        }
    }

    if (pipe_in) {
        if (pipe(fds_to) != 0) {
            throw(error() << "pipe failed");
        } else {
            data->toclose.insert(fds_to[0]);
            data->toclose.insert(fds_to[1]);
        }
    }
    if (pipe_out) {
        if (pipe(fds_from) != 0) {
            throw(error() << "pipe failed");
        } else {
            data->toclose.insert(fds_from[0]);
            data->toclose.insert(fds_from[1]);
        }
    }

    auto env = get_env();

    data->pid = fork();
    pid_t pid = data->pid;

    if (pid < 0) {
        throw(error() << "fork failed");
    } else if (pid == 0) {
        /* child */
        if (pipe_in) {
            close(fds_to[1]);
            data->toclose.erase(fds_to[1]);
            dup2(fds_to[0], 0);
        }

        if (pipe_out) {
            close(fds_from[0]);
            data->toclose.erase(fds_from[0]);
            dup2(fds_from[1], 1);
            dup2(fds_from[1], 2);
        }

        if (!workdir.empty()) {
            chdir(workdir.c_str());
        }

        std::vector<char *> args_vec(argv.size() + 1);
        for (size_t i = 0; i < argv.size(); ++i) {
            args_vec[i] = const_cast<char *>(argv[i].c_str());
        }
        args_vec.back() = nullptr;
        auto args = &args_vec[0];

        std::vector<char *> env_vec(env.size() + 1);
        for (size_t i = 0; i < env.size(); ++i) {
            env_vec[i] = const_cast<char *>(env[i].c_str());
        }
        env_vec.back() = nullptr;
        auto envp = &env_vec[0];

        execve(executable.c_str(), args, envp);

        if (pipe_in) {
            close(0);
        }
        if (pipe_out) {
            close(1);
            close(2);
        }
        return res;
    }

    if (pipe_in) {
        close(fds_to[0]);
    }

    if (pipe_out) {
        close(fds_from[1]);
    }

    auto impl = data.release();
    impl->child_in = fds_to[1];
    impl->child_out = fds_from[0];

    res.reset(new SubprocessInfo(reinterpret_cast<uintptr_t>(impl)));

    return res;
}

int SubprocessInfo::id() const { return D(impl_)->pid; }

#endif // WIN32

std::vector<std::string> get_env()
{
    std::vector<std::string> ret;
    std::set<std::string> seen;
    auto env = Glib::listenv();
    for (const auto &k : env) {
        if (k.find("ART_restore_") == 0) {
            auto key = k.substr(12);
            seen.insert(key);
            auto val = Glib::getenv(k);
            if (!val.empty()) {
                ret.push_back(key + "=" + val);
            }
        }
    }
    for (const auto &key : env) {
        if (key.find("ART_restore_") != 0 && seen.find(key) == seen.end()) {
            auto val = Glib::getenv(key);
            ret.push_back(key + "=" + val);
        }
    }
    return ret;
}

} // namespace subprocess
} // namespace rtengine
