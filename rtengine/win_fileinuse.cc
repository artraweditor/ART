/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright (c) 2026 Alberto Griggio <alberto.griggio@gmail.com>
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

#include "win_fileinuse.h"

#include <string>

#ifdef WIN32

#include <memory>
#include <windows.h>

namespace rtengine {

namespace {

// The Restart Manager bits we need. We declare them ourselves and resolve them
// at run time instead of including <restartmanager.h> and linking rstrtmgr,
// because ART is built with WINVER=0x0501 while the API is Vista+, and because
// a missing DLL must degrade to "no information available" rather than to a
// startup failure.

const int RM_MAX_APP_NAME = 255;
const int RM_MAX_SVC_NAME = 63;
const int RM_SESSION_KEY_CHARS = sizeof(GUID) * 2;

struct RM_UNIQUE_PROCESS_ {
    DWORD dwProcessId;
    FILETIME ProcessStartTime;
};

struct RM_PROCESS_INFO_ {
    RM_UNIQUE_PROCESS_ Process;
    WCHAR strAppName[RM_MAX_APP_NAME + 1];
    WCHAR strServiceShortName[RM_MAX_SVC_NAME + 1];
    int ApplicationType;
    ULONG AppStatus;
    DWORD TSSessionId;
    BOOL bRestartable;
};

typedef DWORD(WINAPI *RmStartSession_t)(DWORD *, DWORD, WCHAR *);
typedef DWORD(WINAPI *RmRegisterResources_t)(DWORD, UINT, LPCWSTR *, UINT,
                                             RM_UNIQUE_PROCESS_ *, UINT,
                                             LPCWSTR *);
typedef DWORD(WINAPI *RmGetList_t)(DWORD, UINT *, UINT *, RM_PROCESS_INFO_ *,
                                   LPDWORD);
typedef DWORD(WINAPI *RmEndSession_t)(DWORD);

typedef DWORD(WINAPI *GetFinalPathNameByHandleW_t)(HANDLE, LPWSTR, DWORD,
                                                   DWORD);

struct RmApi {
    RmStartSession_t start;
    RmRegisterResources_t reg;
    RmGetList_t list;
    RmEndSession_t end;

    RmApi(): start(nullptr), reg(nullptr), list(nullptr), end(nullptr)
    {
        HMODULE m = LoadLibraryW(L"rstrtmgr.dll");
        if (!m) {
            return;
        }
        start = reinterpret_cast<RmStartSession_t>(
            GetProcAddress(m, "RmStartSession"));
        reg = reinterpret_cast<RmRegisterResources_t>(
            GetProcAddress(m, "RmRegisterResources"));
        list = reinterpret_cast<RmGetList_t>(GetProcAddress(m, "RmGetList"));
        end =
            reinterpret_cast<RmEndSession_t>(GetProcAddress(m, "RmEndSession"));
        if (!start || !reg || !list || !end) {
            start = nullptr;
            reg = nullptr;
            list = nullptr;
            end = nullptr;
        }
    }

    bool ok() const { return start != nullptr; }
};

const RmApi &rm_api()
{
    static const RmApi api;
    return api;
}

GetFinalPathNameByHandleW_t final_path_api()
{
    static GetFinalPathNameByHandleW_t f = []() {
        HMODULE m = GetModuleHandleW(L"kernel32.dll");
        return m ? reinterpret_cast<GetFinalPathNameByHandleW_t>(
                       GetProcAddress(m, "GetFinalPathNameByHandleW"))
                 : nullptr;
    }();
    return f;
}

Glib::ustring from_wide(const wchar_t *s)
{
    Glib::ustring ret;
    if (!s || !*s) {
        return ret;
    }
    gchar *u = g_utf16_to_utf8(reinterpret_cast<const gunichar2 *>(s), -1,
                               nullptr, nullptr, nullptr);
    if (u) {
        ret = u;
        g_free(u);
    }
    return ret;
}

typedef std::unique_ptr<wchar_t, void (*)(void *)> wide_str;

wide_str to_wide(const Glib::ustring &s)
{
    return wide_str(reinterpret_cast<wchar_t *>(g_utf8_to_utf16(
                        s.c_str(), -1, nullptr, nullptr, nullptr)),
                    [](void *p) { g_free(p); });
}

} // namespace

std::vector<FileInUseInfo> get_file_holders(const Glib::ustring &fname)
{
    std::vector<FileInUseInfo> ret;

    const RmApi &api = rm_api();
    if (!api.ok()) {
        return ret;
    }

    auto wname = to_wide(fname);
    if (!wname) {
        return ret;
    }

    DWORD session = 0;
    WCHAR key[RM_SESSION_KEY_CHARS + 1];
    memset(key, 0, sizeof(key));

    if (api.start(&session, 0, key) != ERROR_SUCCESS) {
        return ret;
    }

    LPCWSTR files[1] = {wname.get()};
    if (api.reg(session, 1, files, 0, nullptr, 0, nullptr) == ERROR_SUCCESS) {
        std::vector<RM_PROCESS_INFO_> procs(8);
        DWORD reason = 0;
        DWORD err = ERROR_MORE_DATA;

        for (int attempt = 0; attempt < 4 && err == ERROR_MORE_DATA;
             ++attempt) {
            UINT needed = 0;
            UINT got = UINT(procs.size());
            err = api.list(session, &needed, &got, procs.data(), &reason);
            if (err == ERROR_SUCCESS) {
                procs.resize(got);
            } else if (err == ERROR_MORE_DATA) {
                procs.resize(size_t(needed) > procs.size()
                                 ? size_t(needed)
                                 : procs.size() * 2);
            }
        }

        if (err == ERROR_SUCCESS) {
            const DWORD self = GetCurrentProcessId();
            for (const auto &p : procs) {
                FileInUseInfo info;
                info.pid = p.Process.dwProcessId;
                info.is_self = (p.Process.dwProcessId == self);
                info.app_name = from_wide(p.strAppName);
                if (info.app_name.empty()) {
                    info.app_name = "?";
                }
                ret.push_back(info);
            }
        }
    }

    api.end(session);

    return ret;
}

std::vector<Glib::ustring> get_own_open_files()
{
    std::vector<Glib::ustring> ret;

    GetFinalPathNameByHandleW_t get_path = final_path_api();
    if (!get_path) {
        return ret;
    }

    // Handle values are multiples of 4 starting at 4. There is no documented
    // way to enumerate our own handles, but probing the range is harmless as
    // long as we ask GetFileType() first and only query the path of something
    // that really is a file on disk: asking a pipe for its name can block.
    std::vector<wchar_t> buf(1024);
    for (uintptr_t h = 4; h < 64 * 1024; h += 4) {
        HANDLE handle = reinterpret_cast<HANDLE>(h);
        if (GetFileType(handle) != FILE_TYPE_DISK) {
            continue;
        }
        DWORD n = get_path(handle, buf.data(), DWORD(buf.size() - 1), 0);
        if (n >= buf.size()) {
            buf.resize(n + 1);
            n = get_path(handle, buf.data(), DWORD(buf.size() - 1), 0);
        }
        if (n == 0 || n >= buf.size()) {
            continue;
        }
        buf[n] = 0;
        auto path = from_wide(buf.data());
        // GetFinalPathNameByHandle returns the \\?\ form
        if (path.length() > 4 && path.substr(0, 4) == "\\\\?\\") {
            path = path.substr(4);
        }
        if (!path.empty()) {
            ret.push_back(path);
        }
    }

    return ret;
}

} // namespace rtengine

#else // WIN32

namespace rtengine {

std::vector<FileInUseInfo> get_file_holders(const Glib::ustring &)
{
    return std::vector<FileInUseInfo>();
}

std::vector<Glib::ustring> get_own_open_files()
{
    return std::vector<Glib::ustring>();
}

} // namespace rtengine

#endif // WIN32

namespace rtengine {

Glib::ustring format_file_holders(const std::vector<FileInUseInfo> &holders)
{
    Glib::ustring ret;
    for (const auto &h : holders) {
        if (!ret.empty()) {
            ret += ", ";
        }
        ret += Glib::ustring::compose("%1 (%2)", h.app_name,
                                      std::to_string(h.pid));
    }
    return ret;
}

} // namespace rtengine
