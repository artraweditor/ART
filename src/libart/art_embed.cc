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

/*
 *  libART: the in-process backend. The commands are executed in the calling
 *  process, by the same ServerCore that `ART-cli --server` uses behind its
 *  socket; the text they print goes to the callbacks of the engine.
 *
 *  Nothing here uses sockets, fork or threads of its own: the engine's worker
 *  threads never call the user's callbacks (every command completes on the
 *  calling thread).
 */

#include "art_internal.h"

#include "../engine/rtengine.h"
#include "../gui/bootstrap.h"
#include "../gui/options.h"
#include "../engine/remote/servercore.h"
#include <cstdlib>
#include <giomm.h>
#include <glibmm.h>
#include <memory>
#include <mutex>
#include <string>

namespace {

namespace remote = art::engine::remote;
using art::gui::options;

// ---- the engine of the process --------------------------------------------
//
// The options, the engine settings, the thread pool and the GPU context are
// process-wide. The first in-process engine initialises them (with its
// directories and GPU setting); the last one to be destroyed cleans them up.

std::mutex g_mutex;
int g_refs = 0;

bool dir_exists(const std::string &d)
{
    return Glib::file_test(d, Glib::FILE_TEST_IS_DIR);
}

int acquire(const ART_config *cfg, char *err, size_t errsize)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_refs > 0) {
        ++g_refs;
        return ART_OK;
    }

    try {
        Gio::init();

        std::string data;
        if (cfg->data_dir && *cfg->data_dir) {
            data = cfg->data_dir;
        } else if (const char *env = getenv("ART_DATA_DIR")) {
            data = env;
        }
        if (data.empty()) {
            ART_format_error(err, errsize,
                             "the directory of the data files is not known: "
                             "give a data_dir or set ART_DATA_DIR");
            return ART_ERR_INIT;
        }
        if (!dir_exists(data)) {
            ART_format_error(err, errsize, "not a directory: %s", data.c_str());
            return ART_ERR_INIT;
        }
        options.ART_base_dir = data;
        const std::string lensfun = Glib::build_filename(data, "lensfun");
        if (dir_exists(lensfun)) {
            options.rtSettings.lensfunDbDirectory = lensfun;
        }

        // the configuration and cache directories, and the GPU use, are
        // chosen through the environment, as for the programs
        if (cfg->config_dir && *cfg->config_dir) {
            Glib::setenv("ART_SETTINGS", cfg->config_dir, true);
        }
        if (cfg->cache_dir && *cfg->cache_dir) {
            Glib::setenv("ART_CACHE", cfg->cache_dir, true);
        }
        if (cfg->gpu && *cfg->gpu) {
            Glib::setenv("ART_GPU", cfg->gpu, true);
        }

        std::string msg;
        if (art::gui::bootstrap::init(false, cfg->verbose, &msg) != 0) {
            ART_format_error(err, errsize, "cannot load the options: %s",
                             msg.c_str());
            return ART_ERR_INIT;
        }
        if (cfg->embed_params >= 0) {
            options.params_out_embed = cfg->embed_params != 0;
        }
    } catch (std::exception &e) {
        ART_format_error(err, errsize, "cannot initialise the engine: %s",
                         e.what());
        return ART_ERR_INIT;
    } catch (Glib::Error &e) {
        ART_format_error(err, errsize, "cannot initialise the engine: %s",
                         e.what().c_str());
        return ART_ERR_INIT;
    } catch (...) {
        ART_format_error(err, errsize, "cannot initialise the engine");
        return ART_ERR_INIT;
    }
    g_refs = 1;
    return ART_OK;
}

void release()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_refs > 0 && --g_refs == 0) {
        art::engine::cleanup();
    }
}


// ---- executing commands ---------------------------------------------------

// the text of a command goes to the callbacks of the engine
class CallbackSink: public remote::ReplySink {
public:
    explicit CallbackSink(const ART_sink *s): sink_(s), status_(-1) {}

    void out(const std::string &s) override
    {
        if (sink_->out && !s.empty()) {
            sink_->out(sink_->out_user, s.c_str(), s.size());
        }
    }
    void err(const std::string &s) override
    {
        if (sink_->err && !s.empty()) {
            sink_->err(sink_->err_user, s.c_str(), s.size());
        }
    }
    void finish(int status) override { status_ = status; }

    int status() const { return status_; }

private:
    const ART_sink *sink_;
    int status_;
};

struct EmbedState {
    EmbedState(): core(art::gui::bootstrap::editSessionHooks(), "") {}
    art::engine::ServerCore core;
};

int embed_exec(void *state, const ART_config *, const char *cwd, int argc,
               const char *const *argv, const ART_sink *sink, char *err,
               size_t errsize)
{
    EmbedState *st = static_cast<EmbedState *>(state);
    try {
        std::vector<std::string> args(argv, argv + argc);
        auto s = std::make_shared<CallbackSink>(sink);
        st->core.execute(args, cwd, s);
        if (s->status() < 0) {
            ART_format_error(err, errsize, "the command did not complete");
            return ART_ERR_INTERNAL;
        }
        return s->status() == 0 ? ART_OK : ART_ERR_COMMAND;
    } catch (std::exception &e) {
        ART_format_error(err, errsize, "internal error: %s", e.what());
    } catch (...) {
        ART_format_error(err, errsize, "internal error");
    }
    return ART_ERR_INTERNAL;
}

void embed_destroy(void *state)
{
    EmbedState *st = static_cast<EmbedState *>(state);
    try {
        st->core.session().close();
    } catch (...) {
    }
    delete st;
    release();
}

const ART_backend_ops embed_ops = {embed_exec, embed_destroy};

} // namespace


extern "C" int ART_embed_available(void) { return 1; }

extern "C" int ART_embed_create(const ART_config *cfg,
                                const ART_backend_ops **ops, void **state,
                                char *err, size_t errsize)
{
    int rc = acquire(cfg, err, errsize);
    if (rc != ART_OK) {
        return rc;
    }
    try {
        *state = new EmbedState();
    } catch (...) {
        release();
        ART_format_error(err, errsize, "out of memory");
        return ART_ERR_NOMEM;
    }
    *ops = &embed_ops;
    return ART_OK;
}
