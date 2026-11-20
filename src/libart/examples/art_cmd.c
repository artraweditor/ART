/*
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
 *  An example use of libART, and a tool for testing it: runs one command of
 *  the server mode, like `ART-cli --cmd` does.
 *
 *    art_cmd [options] COMMAND [ARGS...]
 *      --socket PATH     remote backend, server on PATH (default: the usual one)
 *      --inprocess       in-process backend (if built in)
 *      --data DIR --config DIR --cache DIR     for --inprocess
 *      --gpu MODE        for --inprocess ("auto", "off", ...)
 *      --timeout MS      remote: give up after MS milliseconds
 *      -V                verbose engine diagnostics (in-process)
 *    The exit status is that of the library call (0, 1 if the command failed,
 *    ...), and errors of the library are printed on stderr.
 */

#include "ART.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    ART_options *o = ART_options_new();
    ART_engine *e = NULL;
    const char *socket_path = NULL, *data = NULL, *config = NULL, *cache = NULL;
    int inprocess = 0, i, rc;

    if (!o) {
        return ART_ERR_NOMEM;
    }
    for (i = 1; i < argc && argv[i][0] == '-' && strcmp(argv[i], "--") != 0; ++i) {
        const char *a = argv[i];
        const char *v = i + 1 < argc ? argv[i + 1] : NULL;
        if (!strcmp(a, "--socket") && v) {
            socket_path = v;
            ++i;
        } else if (!strcmp(a, "--inprocess")) {
            inprocess = 1;
        } else if (!strcmp(a, "--data") && v) {
            data = v;
            ++i;
        } else if (!strcmp(a, "--config") && v) {
            config = v;
            ++i;
        } else if (!strcmp(a, "--cache") && v) {
            cache = v;
            ++i;
        } else if (!strcmp(a, "--gpu") && v) {
            ART_options_set_gpu(o, v);
            ++i;
        } else if (!strcmp(a, "--timeout") && v) {
            ART_options_set_timeout(o, atoi(v));
            ++i;
        } else if (!strcmp(a, "-V")) {
            ART_options_set_verbose(o, 1);
        } else {
            fprintf(stderr, "usage: art_cmd [options] COMMAND [ARGS...] (see the source)\n");
            return 2;
        }
    }
    if (i < argc && !strcmp(argv[i], "--")) {
        ++i;
    }
    if (i >= argc) {
        fprintf(stderr, "art_cmd: no command\n");
        return 2;
    }
    if (inprocess) {
        ART_options_set_inprocess(o, data, config, cache);
    } else {
        ART_options_set_remote(o, socket_path);
    }

    e = ART_engine_create(o);
    if (!e) {
        rc = ART_options_error_code(o);
        fprintf(stderr, "art_cmd: %s: %s\n", ART_strerror(rc), ART_options_error(o));
        ART_options_free(o);
        return rc ? rc : ART_ERR_INTERNAL;
    }
    ART_options_free(o);

    /* argv[i..argc-1] followed by the NULL that argv already has */
    rc = ART_exec_cmd(e, (const char *const *)&argv[i]);
    if (rc != ART_OK && rc != ART_ERR_COMMAND) {
        fprintf(stderr, "art_cmd: %s: %s\n", ART_strerror(rc), ART_engine_error(e));
    }
    ART_engine_destroy(e);
    return rc;
}
