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

#include "servermode.h"
#include "../engine/remote/command_dispatch.h"
#include "../engine/remote/ipc_transport.h"
#include "../engine/remote/wire.h"
#include "servercore.h"
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <glibmm.h>
#include <iostream>
#include <unistd.h>

namespace art { namespace gui { namespace servermode {

namespace remote = art::engine::remote;

namespace {

const char *const HELP_HINT = "try: ART-cli --cmd help";

// ---- client ----------------------------------------------------------------

// a failure of the client itself (no server, broken connection): reported as
// the server reports a failed command, a JSON document on stdout and the
// message on stderr
int fail_client(const std::string &msg)
{
    std::cout << remote::failureDocument(msg) << std::flush;
    std::cerr << "ART: error: " << msg << std::endl;
    return 1;
}


// ---- server ----------------------------------------------------------------

volatile std::sig_atomic_t g_stop = 0;

extern "C" void on_signal(int) { g_stop = 1; }


// the output of a command goes back to the client as frames
class SocketSink: public remote::ReplySink {
public:
    explicit SocketSink(remote::ipc::Connection &c): c_(c), alive_(true), finished_(false) {}

    void out(const std::string &s) override
    {
        alive_ = alive_ && remote::wire::writeFrame(c_, 'O', s);
    }
    void err(const std::string &s) override
    {
        alive_ = alive_ && remote::wire::writeFrame(c_, 'E', s);
    }
    void finish(int status) override
    {
        if (!finished_) {
            finished_ = true;
            alive_ = alive_ && remote::wire::writeStatus(c_, status);
        }
    }
    bool finished() const { return finished_; }

private:
    remote::ipc::Connection &c_;
    bool alive_;
    bool finished_;
};


void serve(remote::ipc::Connection &conn, ServerCore &core, int verbose)
{
    remote::wire::Request req;
    std::string err;
    auto sink = std::make_shared<SocketSink>(conn);
    if (!remote::wire::readRequest(conn, req, err)) {
        if (verbose) {
            std::cerr << "ART-cli server: bad request: " << err << std::endl;
        }
        remote::Reply(sink).fail(err);
        return;
    }
    if (verbose) {
        std::cerr << "ART-cli server: command:";
        for (auto &a : req.args) {
            std::cerr << " " << a;
        }
        std::cerr << std::endl;
    }
    core.execute(req.args, req.cwd, sink);
}

} // namespace


bool parseArgs(int argc, char **argv, Options &opts, std::string &err)
{
    opts = Options();
    int i = 1;
    std::string socket;
    if (i + 1 < argc && std::string(argv[i]) == "--socket") {
        socket = argv[i + 1];
        i += 2;
    }
    if (i >= argc) {
        return false;
    }
    const std::string a = argv[i];
    if (a == "--server") {
        if (i + 1 < argc && std::string(argv[i + 1]) != "-V" &&
            std::string(argv[i + 1]) != "-q") {
            err = "unexpected argument after --server: " +
                  std::string(argv[i + 1]);
            return false;
        }
        opts.mode = Options::SERVER;
        opts.socket = socket;
        return true;
    }
    if (a == "--cmd") {
        opts.mode = Options::CLIENT;
        opts.socket = socket;
        for (int j = i + 1; j < argc; ++j) {
            opts.cmd.push_back(argv[j]);
        }
        return true;
    }
    return false;
}


void printHelp(std::ostream &out)
{
    out << "  ART-cli [--socket PATH] --server [-V]\n"
        << "      Run a server that keeps one image open and executes the commands\n"
        << "      sent by other ART-cli processes. It listens on a Unix domain socket\n"
        << "      (PATH, else $ART_SERVER_SOCKET, else server.sock in a private\n"
        << "      directory under $XDG_RUNTIME_DIR or $TMPDIR). Nothing is written to\n"
        << "      the image's sidecar until 'save'.\n"
        << "  ART-cli [--socket PATH] --cmd <command> [args]\n"
        << "      Send a command to the server (it fails if none is running). It prints\n"
        << "      one line of JSON on stdout: {\"ok\":true,\"result\":...} or\n"
        << "      {\"ok\":false,\"error\":...}. Commands:\n"
        << "    open FILE                    Open FILE and make it the active image\n"
        << "    save                         Write the parameters and snapshots to the sidecar\n"
        << "    get-files                    Print the active image\n"
        << "    apply [--append] [--merge KEY] PROFILE\n"
        << "                                 Apply a (partial) profile to the active image\n"
        << "    apply :neutral | :default    Apply the neutral profile, or the default profile of the\n"
        << "                                 preferences (possibly a dynamic one), replacing all the parameters\n"
        << "    get-params                   Print the current parameters (.arp text)\n"
        << "    get-info                     Print size, camera and exposure information\n"
        << "    get-metadata [--makernotes | --no-makernotes] [--edits | --no-edits]\n"
        << "                                 Print the metadata (Exif tags with their types; maker notes as\n"
        << "                                 strings) as a JSON object, with or without the edits of the profile\n"
        << "    get-history                  List the edit history (index, * = selected, text, value)\n"
        << "    undo [N] | redo [N]          Step back/forward in the history\n"
        << "    history-goto INDEX           Select a history entry\n"
        << "    get-snapshots                List the snapshots (* = active)\n"
        << "    snapshot-activate NAME       Activate a snapshot\n"
        << "    snapshot-add [NAME]          Save the current state as a new snapshot\n"
        << "    snapshot-overwrite NAME      Replace a snapshot with the current state\n"
        << "    snapshot-remove NAME         Delete a snapshot\n"
        << "    preview OUT [--scale F] [--crop X,Y,W,H]\n"
        << "                                 Render the image (crop in full-image\n"
        << "                                 coordinates) to a PNG/JPEG file\n"
        << "    export OUTFILE [--format F] [--quality N] [--subsampling 1-3] [--bits N]\n"
        << "           [--float] [--compression none|zip] [--overwrite] [--fast]\n"
        << "                                 Write the image at full size, as ART-cli would; the format is\n"
        << "                                 that of OUTFILE unless --format is given\n"
        << "    export --list-formats        Print the output formats\n"
        << "    get-spot-wb X Y W H          Print the white balance (temperature, tint, multipliers)\n"
        << "                                 that makes the rectangle (full-image coordinates) neutral\n"
        << "    get-clut-params FILE         Print the parameters of a LUT or CTL script (JSON)\n"
        << "    ping                         Check that the server answers\n"
        << "    quit                         Stop the server\n";
}


int runClient(const Options &opts)
{
    if (!opts.cmd.empty() && opts.cmd[0] == "help") {
        printHelp(std::cout);
        return 0;
    }

    std::string err;
    std::string path = opts.socket;
    if (path.empty()) {
        path = remote::ipc::defaultSocketPath(err);
        if (path.empty()) {
            return fail_client(err);
        }
    }
    auto conn = remote::ipc::connect(path, err);
    if (!conn) {
        return fail_client("no running server (" + err + ")");
    }

    remote::wire::Request req;
    char buf[4096];
    if (getcwd(buf, sizeof(buf))) {
        req.cwd = buf;
    }
    req.args = opts.cmd;
    if (!remote::wire::writeRequest(*conn, req)) {
        return fail_client("cannot send the command to the server");
    }

    int status = 1;
    const bool ok = remote::wire::readResponse(
        *conn,
        [](char kind, const std::string &data) {
            FILE *f = kind == 'O' ? stdout : stderr;
            fwrite(data.data(), 1, data.size(), f);
            fflush(f);
        },
        status, err);
    if (!ok) {
        return fail_client(err);
    }
    return status;
}


int runServer(const Options &opts, int verbose)
{
    std::string err;
    std::string path = opts.socket;
    if (path.empty()) {
        path = remote::ipc::defaultSocketPath(err);
        if (path.empty()) {
            std::cerr << "ART-cli: error: " << err << std::endl;
            return 1;
        }
    }
    auto listener = remote::ipc::listen(path, err);
    if (!listener) {
        std::cerr << "ART-cli: error: " << err << std::endl;
        return 1;
    }

    // no SA_RESTART: a signal must interrupt the wait for a client
    struct sigaction sa;
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    signal(SIGPIPE, SIG_IGN);

    ServerCore core(HELP_HINT);
    bool quit = false;
    core.setQuitHandler([&quit]() { quit = true; });

    // scripts wait for this line before sending commands
    std::cout << "ART-cli server listening on " << path << std::endl;

    while (!quit && !g_stop) {
        auto conn = listener->accept(500);
        if (conn) {
            serve(*conn, core, verbose);
        }
    }
    if (verbose) {
        std::cerr << "ART-cli server: exiting" << std::endl;
    }
    core.session().close();
    return 0;
}

}}} // namespace art::gui::servermode
