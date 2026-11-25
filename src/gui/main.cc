/* -*- C++ -*-
 *  This file is part of ART.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *  Copyright (c) 2017-2026 Alberto Griggio <alberto.griggio@gmail.com>
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

#ifdef __GNUC__
#if defined(__FAST_MATH__)
#error Using the -ffast-math CFLAG is known to lead to problems. Disable it to compile ART.
#endif
#endif

#include "../engine/dynamicprofile.h"
#include "../utils/config.h"
#include "extprog.h"
#include "options.h"
#include "../utils/pathutils.h"
#include "printhelp.h"
#include "rtimage.h"
#include "../engine/remote/remote_reply.h"
#include "remotecmd.h"
#include "rtwindow.h"
#include "session.h"
#include "shortcutsmanager.h"
#include "soundman.h"
#include "../utils/version.h"
#include "wbprovider.h"
#include <cstdlib>
#include <cstring>
#include <giomm.h>
#include <gtkmm.h>
#include <iostream>
#include <lensfun.h>
#include <locale.h>
#include <tiffio.h>

#ifndef WIN32
#include <glib.h>
#include <glib/gstdio.h>
#include <glibmm/fileutils.h>
#include <glibmm/threads.h>
#else // WIN32
#include "conio.h"
#include <glibmm/thread.h>
#endif

#include <csignal>
#include <gio/gio.h>

#ifdef WITH_MIMALLOC
#include <mimalloc.h>
#endif

#ifdef __APPLE__
#include <gtkosxapplication.h>
#endif

namespace art { namespace gui {
using namespace utils;


extern Options options;

// stores path to data files
Glib::ustring creditsPath;
Glib::ustring licensePath;
Glib::ustring argv1;
Glib::ustring argv2;
bool simpleEditor = false;
bool gimpPlugin = false;
bool remote = false;
bool is_session = false;
// --cmd mode: the command (verb and arguments) to send to the running instance
bool cmd_mode = false;
// the -S* session options are translated to commands, but (unlike --cmd) they
// still start the GUI when no instance is running
bool cmd_start_ok = false;
std::vector<Glib::ustring> cmd_args;

namespace {

// This recursive mutex will be used by gdk_threads_enter/leave instead of a
// simple mutex
static Glib::Threads::RecMutex myGdkRecMutex;

static void myGdkLockEnter() { myGdkRecMutex.lock(); }

static void myGdkLockLeave() { myGdkRecMutex.unlock(); }

void process_help_params(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        Glib::ustring currParam(argv[i]);

        if (currParam.length() > 1 && currParam[0] == '-') {
            switch (currParam[1]) {
            case 'v':
                std::cout << ART_NAME << ", version " << ART_VERSION << std::endl;
                exit(0);
            case '?':
            case 'h':
                ART_print_help(argv[0], true);
                exit(0);
            default:
                break;
            }
        }
    }
}

/* Process line command options
 * Returns
 *  0 if process in batch has executed
 *  1 to start GUI (with a dir or file option)
 *  2 to start GUI because no files found
 *  -1 if there is an error in parameters
 *  -2 if an error occurred during processing
 *  -3 if at least one required procparam file was not found */
int processLineParams(int argc, char **argv)
{
    Glib::setenv("ART_IS_SESSION", "0");
    Glib::setenv("ART_IS_CMD", "0");
    int ret = 1;
    for (int iArg = 1; iArg < argc; iArg++) {
        Glib::ustring currParam(argv[iArg]);
        if (currParam.empty()) {
            continue;
        }

        if (currParam[0] == '-' && currParam.size() > 1) {
            switch (currParam[1]) {
#ifdef WIN32

            case 'w': // This case is handled outside this function
                break;
#endif

            case 'v':
                printf("%s, version %s\n", ART_NAME, ART_VERSION);
                ret = 0;
                break;

            case 'N':
                remote = false;
                break;

            case 's':
                simpleEditor = true;
                remote = false;
                break;

            case '-':
                if (currParam == "--cmd") {
                    // everything after --cmd is the command and its arguments
                    cmd_mode = true;
                    for (++iArg; iArg < argc; ++iArg) {
                        cmd_args.emplace_back(fname_to_utf8(argv[iArg]));
                    }
                    if (cmd_args.size() == 1 && cmd_args[0] == "help") {
                        ART_print_help(argv[0], true);
                        ret = 0;
                    }
                    break;
                }
                if (currParam.substr(0, 5) == "--gtk" ||
                    currParam == "--g-fatal-warnings") {
                    break;
                }

            case 'g':
                if (currParam == "-gimp") {
                    gimpPlugin = true;
                    simpleEditor = true;
                    remote = false;
                    break;
                }

            case 'S':
                if (remote) {
                    // translated into the equivalent --cmd session command
                    const auto collect = [&](bool &any) {
                        ++iArg;
                        while (iArg < argc && argv[iArg][0] != '-') {
                            cmd_args.emplace_back(fname_to_utf8(argv[iArg]));
                            any = true;
                            ++iArg;
                        }
                        --iArg;
                    };
                    bool any = false;
                    if (currParam == "-S") {
                        cmd_args = {"session", "load"};
                        if (iArg + 1 < argc && argv[iArg + 1][0] != '-') {
                            collect(any);
                        }
                    } else if (currParam == "-Sc") {
                        cmd_args = {"session", "clear"};
                    } else if (currParam == "-Sa") {
                        cmd_args = {"session", "add"};
                        collect(any);
                    } else if (currParam == "-Sr") {
                        cmd_args = {"session", "remove"};
                        collect(any);
                    } else if (currParam == "-So") {
                        cmd_args = {"session", "open"};
                        collect(any);
                    }
                    if (!cmd_args.empty()) {
                        // old behaviour: switch to the session view, start
                        // the GUI if needed
                        if (cmd_args[1] != "open") {
                            cmd_args.insert(cmd_args.begin() + 1, "--show");
                        }
                        cmd_mode = true;
                        cmd_start_ok = true;
                        break;
                    }
                }
                // no break here on purpose

            case 'h':
            case '?':
            default:
                ART_print_help(argv[0], true);
                ret = -1;
                break;
            }
        } else {
            if (argv1.empty()) {
                argv1 = Glib::ustring(fname_to_utf8(argv[iArg]));
            } else if (gimpPlugin) {
                argv2 = Glib::ustring(fname_to_utf8(argv[iArg]));
                break;
            }

            if (!gimpPlugin) {
                break;
            }
        }
    }

    return ret;
}

bool init_rt()
{
    UserCommandStore::getInstance()->init(
        Glib::build_filename(options.user_config_dir, "usercommands"));
    SoundManager::init();
    wb_presets::init(options.ART_base_dir, options.user_config_dir);
    ShortcutsManager::getInstance()->init(options.ART_base_dir,
                                          options.user_config_dir);

    if (!options.rtSettings.verbose) {
        TIFFSetWarningHandler(nullptr); // avoid annoying message boxes
    }

#ifndef WIN32

    // Move the old path to the new one if the new does not exist
    if (Glib::file_test(Glib::build_filename(options.user_config_dir, "cache"),
                        Glib::FILE_TEST_IS_DIR) &&
        !Glib::file_test(options.cacheBaseDir, Glib::FILE_TEST_IS_DIR)) {
        g_rename(Glib::build_filename(options.user_config_dir, "cache").c_str(),
                 options.cacheBaseDir.c_str());
    }

#endif

    return true;
}

void cleanup_rt() { art::engine::cleanup(); }

RTWindow *create_rt_window()
{
    Glib::ustring icon_path =
        Glib::build_filename(options.ART_base_dir, "images");
    Glib::RefPtr<Gtk::IconTheme> defaultIconTheme =
        Gtk::IconTheme::get_default();
    defaultIconTheme->append_search_path(icon_path);
    // override the stock GTK folder icons (places, file chooser, ...)
    updateFolderIconTheme();

    // gdk_threads_enter ();
    RTWindow *rtWindow = new RTWindow();
    return rtWindow;
}

#ifdef __APPLE__

static gboolean osx_open_file_cb(GtkosxApplication *a, gchar *pth,
                                 gpointer data);

#endif // __APPLE__

namespace {

/**
 * The macOS (and Windows/Linux) bundle launchers start a private D-Bus session
 * daemon for ART and flag it with ART_PRIVATE_DBUS. Nothing else ever stops it,
 * so it would outlive ART, running in the background after quitting.
 *
 * The primary instance (i.e. not one that just forwarded its command line to a
 * running one) stops it when it is done. The pid must be fetched while the bus
 * is up, and the daemon killed only after the application object is gone:
 * GDBus exits the process as soon as the session bus connection is closed,
 * which would skip the shutdown work done in ~RTApplication.
 */
class PrivateBusGuard {
public:
    PrivateBusGuard()
    {
        if (!g_getenv("ART_PRIVATE_DBUS")) {
            return;
        }
        GError *err = nullptr;
        GDBusConnection *c = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &err);
        if (!c) {
            g_clear_error(&err);
            return;
        }
        GVariant *r = g_dbus_connection_call_sync(
            c, "org.freedesktop.DBus", "/org/freedesktop/DBus",
            "org.freedesktop.DBus", "GetConnectionUnixProcessID",
            g_variant_new("(s)", "org.freedesktop.DBus"), G_VARIANT_TYPE("(u)"),
            G_DBUS_CALL_FLAGS_NONE, 2000, nullptr, &err);
        if (r) {
            guint32 pid = 0;
            g_variant_get(r, "(u)", &pid);
            g_variant_unref(r);
            pid_ = pid;
        } else {
            g_clear_error(&err);
        }
        g_object_unref(c);
    }

    ~PrivateBusGuard()
    {
        if (stop_ && pid_ > 1) {
            kill(pid_, SIGTERM);
        }
    }

    void stopOnExit(bool yes) { stop_ = yes; }

private:
    pid_t pid_ = 0;
    bool stop_ = false;
};

} // namespace

class RTApplication: public Gtk::Application {
public:
    RTApplication()
        : Gtk::Application("us.pixls.art.ART",
                           Gio::APPLICATION_SEND_ENVIRONMENT |
                               Gio::APPLICATION_HANDLES_COMMAND_LINE),
          rtWindow(nullptr)
    {
#ifdef __APPLE__
        osxApp =
            (GtkosxApplication *)g_object_new(GTKOSX_TYPE_APPLICATION, NULL);
        g_signal_connect(osxApp, "NSApplicationOpenFile",
                         G_CALLBACK(osx_open_file_cb), this);
#endif // __APPLE__
    }

    ~RTApplication() override
    {
        if (rtWindow) {
            art::gui::session::save(art::gui::session::filename() + ".last");
            art::gui::session::clear();
            art::gui::Thumbnail::flushPendingWrites();
            cleanup_rt();
        }
#ifdef __APPLE__
        if (osxApp) {
            g_object_unref(osxApp);
        }
#endif // __APPLE__
    }

private:
    bool create_window()
    {
        if (rtWindow) {
            return true;
        }

        if (!init_rt()) {
            Gtk::MessageDialog msgd(
                "Fatal error!\nThe ART_SETTINGS environment variable is set, "
                "but uses a relative path. The path must be absolute!",
                true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
            add_window(msgd);
            msgd.run();
            return false;
        } else {
            rtWindow = create_rt_window();
            add_window(*rtWindow);
            rtWindow->setApplication(true);
#ifdef __APPLE__
            add_action("quit", sigc::slot<void>([this]() {
                           GThreadLock lck;
                           rtWindow->quit();
                       }));
            add_action("preferences", sigc::slot<void>([this]() {
                           GThreadLock lck;
                           rtWindow->showPreferences();
                       }));
            add_action("about", sigc::slot<void>([this]() {
                           GThreadLock lck;
                           Splash splash(*rtWindow);
                           splash.run();
                       }));
#endif // __APPLE__
            return true;
        }
    }

    int on_command_line(
        const Glib::RefPtr<Gio::ApplicationCommandLine> &command_line) override
    {
        if (atoi(command_line->getenv("ART_IS_CMD").c_str())) {
            int argc = 0;
            auto argv = command_line->get_arguments(argc);
            // argv[0] is the base dir, argv[1] is "--cmd"
            std::vector<Glib::ustring> args;
            for (int i = 2; i < argc; ++i) {
                args.emplace_back(argv[i]);
            }
            g_strfreev(argv);
            const bool raise = !rtWindow;
            if (raise &&
                !atoi(command_line->getenv("ART_CMD_START").c_str())) {
                command_line->print(
                    art::engine::remote::failureDocument("no running instance"));
                command_line->printerr("ART: error: no running instance\n");
                return 1;
            }
            if (!create_window()) {
                return 1;
            }
            if (raise) {
                rtWindow->present();
            }
            remotecmd::run(rtWindow, command_line, args);
            return command_line->get_exit_status();
        }

        auto s = command_line->getenv("ART_IS_SESSION");
        is_session = (!s.empty() && atoi(s.c_str()));

        int argc = 0;
        auto argv = command_line->get_arguments(argc);

        return process_command_line(argc, argv);
    }

public:
    int process_command_line(int argc, char **argv)
    {
        if (is_session) {
            bool raise = !rtWindow;
            if (create_window()) {
                struct Data {
                    Glib::ustring fname;
                    FileCatalog *filecatalog;
                };
                const auto doit = [](gpointer data) -> gboolean {
                    Data *d = static_cast<Data *>(data);
                    FileCatalog *filecatalog = d->filecatalog;
                    Glib::ustring fname = "";
                    if (!d->fname.empty()) {
                        for (auto n : art::gui::session::list()) {
                            if (d->fname == n) {
                                fname = n;
                                break;
                            }
                        }
                    }
                    if (!art::gui::session::check(filecatalog->lastSelectedDir())) {
                        filecatalog->dirSelected(art::gui::session::path(), "");
                    }
                    if (!fname.empty()) {
                        filecatalog->openFile(fname);
                    }
                    delete d;
                    return FALSE;
                };
                Data *d = new Data;
                d->filecatalog = rtWindow->fpanel->fileCatalog;
                d->fname = ((argc > 1) ? argv[1] : "");
                gdk_threads_add_idle(doit, d);
                if (raise) {
                    rtWindow->present();
                }
                return 0;
            }
        } else if (argc == 2) {
            if (create_window()) {
                struct Data {
                    Glib::ustring dirname;
                    Glib::ustring fname;
                    FileCatalog *filecatalog;
                };
                Data *d = new Data;
                d->fname = argv[1];
                if (Glib::file_test(d->fname, Glib::FILE_TEST_IS_DIR)) {
                    d->dirname = d->fname;
                    d->fname = "";
                } else {
                    d->dirname = Glib::path_get_dirname(d->fname);
                }
                d->filecatalog = rtWindow->fpanel->fileCatalog;

                const auto doit = [](gpointer data) -> gboolean {
                    Data *d = static_cast<Data *>(data);
                    d->filecatalog->dirSelected(d->dirname, d->fname);
                    delete d;
                    return FALSE;
                };
                gdk_threads_add_idle(doit, d);
                rtWindow->present();
                return 0;
            }
        } else {
            if (create_window()) {
                rtWindow->present();
                return 0;
            }
        }
        return 1;
    }

    Glib::ustring get_selected_dir()
    {
        if (rtWindow) {
            return rtWindow->fpanel->fileCatalog->lastSelectedDir();
        }
        return "";
    }

private:
    RTWindow *rtWindow;
#ifdef __APPLE__
    GtkosxApplication *osxApp;
#endif // __APPLE__
};

#ifdef __APPLE__

static gboolean osx_open_file_cb(GtkosxApplication *a, gchar *pth,
                                 gpointer data)
{
    GThreadLock lck;

    if (options.rtSettings.verbose > 1) {
        std::cout << "osx_open_file_cb: " << pth << std::endl;
    }
    
    RTApplication *app = (RTApplication *)data;
    int argc = 2;
    auto fname = pth;
    if (strstr(pth, "file://") == pth) {
        fname = g_filename_from_uri(pth, nullptr, nullptr);
        if (!fname) {
            return false;
        }
    }
    char *argv[2] = {nullptr, fname};
    is_session = false;
    auto ret = app->process_command_line(argc, argv);
    if (fname != pth) {
        g_free(fname);
    }
    return ret;
}

#endif // __APPLE__

void show_gimp_plugin_info_dialog(Gtk::Window *parent)
{
    if (options.gimpPluginShowInfoDialog) {
        Gtk::MessageDialog info(*parent, M("GIMP_PLUGIN_INFO"), false,
                                Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
        Gtk::Box *box = info.get_message_area();
        Gtk::CheckButton dontshowagain(M("DONT_SHOW_AGAIN"));
        dontshowagain.show();
        box->pack_start(dontshowagain);
        info.run();
        options.gimpPluginShowInfoDialog = !dontshowagain.get_active();
    }
}

} // namespace



} } // namespace art::gui

using namespace art::gui;

int main(int argc, char **argv)
{
#ifdef WITH_MIMALLOC
    mi_version();
#endif

#ifndef ART_WIN32_UCRT
    setlocale(LC_ALL, "");
#endif
    setlocale(LC_NUMERIC, "C"); // to set decimal point to "."

    simpleEditor = false;
    gimpPlugin = false;
    remote = true;
    options.ART_base_dir = "";
    argv1 = "";
    argv2 = "";

    process_help_params(argc, argv);

    Glib::init(); // called by Gtk::Main, but this may be important for thread
                  // handling, so we call it ourselves now
    Gio::init();

#ifdef WIN32
    if (GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == 0x0003) {
        // started from msys2 console => do not buffer stdout
        setbuf(stdout, NULL);
    }
#endif

#ifdef BUILD_BUNDLE
    Glib::ustring exePath = getExecutablePath(argv[0]);

    // set paths
    if (Glib::path_is_absolute(DATA_SEARCH_PATH)) {
        options.ART_base_dir = DATA_SEARCH_PATH;
    } else if (strcmp(DATA_SEARCH_PATH, ".") == 0) {
        options.ART_base_dir = exePath;
    } else {
        options.ART_base_dir = Glib::build_filename(exePath, DATA_SEARCH_PATH);
    }

    if (Glib::path_is_absolute(CREDITS_SEARCH_PATH)) {
        creditsPath = CREDITS_SEARCH_PATH;
    } else {
        creditsPath = Glib::build_filename(exePath, CREDITS_SEARCH_PATH);
    }

    if (Glib::path_is_absolute(LICENCE_SEARCH_PATH)) {
        licensePath = LICENCE_SEARCH_PATH;
    } else {
        licensePath = Glib::build_filename(exePath, LICENCE_SEARCH_PATH);
    }

    options.rtSettings.lensfunDbDirectory = LENSFUN_DB_PATH;

#else  // BUILD_BUNDLE
    options.ART_base_dir = DATA_SEARCH_PATH;
    creditsPath = CREDITS_SEARCH_PATH;
    licensePath = LICENCE_SEARCH_PATH;
    options.rtSettings.lensfunDbDirectory = LENSFUN_DB_PATH;
#endif // BUILD_BUNDLE

    // `--cmd` is parsed later (process_command_line); the options are loaded
    // before that, so look for it here
    bool cmd_client = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--cmd") == 0) {
            cmd_client = true;
            break;
        }
    }

    Glib::ustring fatalError;
    if (!cmd_client) {
        // a command-mode client only talks to a running instance: it must not
        // load the options (which also prints diagnostics to stdout, where
        // only the reply of the command belongs)
        try {
            Options::load();
        } catch (Options::Error &e) {
            fatalError = e.get_msg();
        }
    }

#ifdef WIN32
    bool consoleOpened = false;

    // suppression of annoying error boxes
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX |
                 SEM_NOOPENFILEERRORBOX);

    if (argc > 1) {
        if (!remote && !Glib::file_test(argv1, Glib::FILE_TEST_EXISTS) &&
            !Glib::file_test(argv1, Glib::FILE_TEST_IS_DIR)) {
            bool stdoutRedirecttoConsole =
                (GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == 0x0000);
            // open console, if stdout is invalid
            if (stdoutRedirecttoConsole) {
                // check if parameter -w was passed.
                // We have to do that in this step, because it controls whether
                // to open a console to show the output of following steps
                bool Console = true;

                for (int i = 1; i < argc; i++)
                    if (!strcmp(argv[i], "-w") || !strcmp(argv[i], "-R") ||
                        !strcmp(argv[i], "-gimp")) {
                        Console = false;
                        break;
                    }

                if (Console && AllocConsole()) {
                    AttachConsole(GetCurrentProcessId());
                    // Don't allow CTRL-C in console to terminate RT
                    SetConsoleCtrlHandler(NULL, true);
                    // Set title of console
                    char consoletitle[128];
                    sprintf(consoletitle, "%s %s Console", ART_NAME, ART_VERSION);
                    SetConsoleTitle(consoletitle);
                    // increase size of screen buffer
                    COORD c;
                    c.X = 200;
                    c.Y = 1000;
                    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),
                                               c);
                    // Disable console-Cursor
                    CONSOLE_CURSOR_INFO cursorInfo;
                    cursorInfo.dwSize = 100;
                    cursorInfo.bVisible = false;
                    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                                         &cursorInfo);

                    if (stdoutRedirecttoConsole) { // if stdout is Redirect to
                                                   // console, we also redirect
                                                   // stderr to console
                        freopen("CON", "w", stdout);
                        freopen("CON", "w", stderr);
                    }

                    freopen("CON", "r", stdin);

                    consoleOpened = true;
                }
            }
        }
        int ret = processLineParams(argc, argv);

        if (ret <= 0) {
            fflush(stdout);
            if (consoleOpened) {
                printf("Press any key to exit " ART_NAME "\n");
                FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
                getch();
            }

            return ret;
        }
    }

#else // WIN32

    if (argc > 1) {
        int ret = processLineParams(argc, argv);

        if (ret <= 0) {
            return ret;
        }
    }

#endif // WIN32

    if (gimpPlugin) {
        if (!Glib::file_test(argv1, Glib::FILE_TEST_EXISTS) ||
            Glib::file_test(argv1, Glib::FILE_TEST_IS_DIR)) {
            printf("Error: argv1 doesn't exist\n");
            return 1;
        }

        if (argv2.empty()) {
            printf("Error: -gimp requires two arguments\n");
            return 1;
        }
    } else if (!remote && !(Glib::file_test(argv1, Glib::FILE_TEST_EXISTS) &&
                            !Glib::file_test(argv1, Glib::FILE_TEST_IS_DIR))) {
        simpleEditor = false;
    }

    int ret = 0;

    gdk_threads_set_lock_functions(G_CALLBACK(myGdkLockEnter),
                                   (G_CALLBACK(myGdkLockLeave)));
    gdk_threads_init();
    gtk_init(&argc, &argv); // use the "--g-fatal-warnings" command line flag to
                            // make warnings fatal
    if ((!cmd_mode || cmd_start_ok) && fatalError.empty()) {
        // a pure remote-command client does not need it
        //
        // also, Options::load() failing means art::engine::init()
        // never ran, so the engine's global settings pointer is null:
        // touching the ICC store would segfault and hide the real
        // error behind the fatal-error dialog below
        initGUIColorManagement();
    }

    if (fatalError.empty() && remote && cmd_mode) {
        // remote command: {base_dir, "--cmd", verb, args...}
        std::vector<std::string> store;
        store.push_back(options.ART_base_dir);
        store.push_back("--cmd");
        for (auto &a : cmd_args) {
            store.push_back(a);
        }
        std::vector<char *> app_argv;
        for (auto &a : store) {
            app_argv.push_back(const_cast<char *>(a.c_str()));
        }
        app_argv.push_back(nullptr);
        Glib::setenv("ART_IS_CMD", "1");
        Glib::setenv("ART_CMD_START", cmd_start_ok ? "1" : "0");

        PrivateBusGuard busGuard;
        RTApplication app;
        if (!cmd_start_ok) {
            // never start a GUI just to run a command
            bool is_remote = false;
            try {
                is_remote = app.register_application() && app.is_remote();
            } catch (Glib::Error &) {
            }
            if (!is_remote) {
                std::cout << art::engine::remote::failureDocument(
                                 "no running instance")
                          << std::flush;
                std::cerr << "ART: error: no running instance" << std::endl;
                return 1;
            }
        }
        ret = app.run(int(app_argv.size()) - 1, app_argv.data());
        busGuard.stopOnExit(!app.is_remote());
    } else if (fatalError.empty() && remote) {
#if 1 // ndef __APPLE__
        char *app_argv[2] = {const_cast<char *>(options.ART_base_dir.c_str())};
        int app_argc = 1;

        if (!argv1.empty()) {
            app_argc = 2;
            app_argv[1] = const_cast<char *>(argv1.c_str());
        }
#else
        auto app_argc = argc;
        auto app_argv = argv;
#endif // __APPLE__

        PrivateBusGuard busGuard;
        RTApplication app;
        ret = app.run(app_argc, app_argv);
        busGuard.stopOnExit(!app.is_remote());
    } else {
        if (fatalError.empty() && init_rt()) {
            Gtk::Main m(&argc, &argv);
            gdk_threads_enter();
            const std::unique_ptr<RTWindow> rtWindow(create_rt_window());
            if (gimpPlugin) {
                show_gimp_plugin_info_dialog(rtWindow.get());
            }
            m.run(*rtWindow);
            gdk_threads_leave();

            if (gimpPlugin && rtWindow->epanel &&
                rtWindow->epanel->isRealized()) {
                if (!rtWindow->epanel->saveImmediately(argv2, SaveFormat())) {
                    ret = -2;
                }
            }

            cleanup_rt();
        } else {
            Gtk::Main m(&argc, &argv);
            Gtk::MessageDialog msgd(
                Glib::ustring::compose("FATAL ERROR!\n\n%1", fatalError), true,
                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
            msgd.run();
            ret = -2;
        }
    }

#ifdef WIN32

    if (consoleOpened) {
        printf("Press any key to exit " ART_NAME "\n");
        fflush(stdout);
        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        getch();
    }

#endif

    return ret;
}
