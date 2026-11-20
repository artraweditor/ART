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
 *  Test of the in-process backend on a real image. It needs the data files
 *  and an image, so it only runs when the environment says where they are:
 *
 *    ART_TEST_DATA_DIR   the directory of ART's data files
 *                        (e.g. build/gpu/Release/ART.app/Contents/Resources)
 *    ART_TEST_IMAGE      a raw file; a copy is edited, never the file itself
 *    ART_TEST_PROFILE    an .arp file to apply (default: IMAGE.arp)
 *
 *  Exit status 77 (skipped by ctest) if the variables are not set.
 *  Also needs the libraries of the engine to be found (DYLD_LIBRARY_PATH).
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1
#define _DARWIN_C_SOURCE 1

#include "ART.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            ++failures;                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                  \
        }                                                                    \
    } while (0)

static int copy_file(const char *from, const char *to)
{
    FILE *a = fopen(from, "rb"), *b;
    char buf[1 << 16];
    size_t n;
    if (!a) {
        return -1;
    }
    b = fopen(to, "wb");
    if (!b) {
        fclose(a);
        return -1;
    }
    while ((n = fread(buf, 1, sizeof(buf), a)) > 0) {
        fwrite(buf, 1, n, b);
    }
    fclose(a);
    fclose(b);
    return 0;
}

/* run a command and return its collected output (static buffer) */
static int run(ART_engine *e, char *out, size_t outsize, const char *a0,
               const char *a1, const char *a2, const char *a3, const char *a4,
               const char *a5)
{
    const char *argv[8];
    int n = 0, rc;
    ART_result *r = NULL;
    argv[n++] = a0;
    if (a1) argv[n++] = a1;
    if (a2) argv[n++] = a2;
    if (a3) argv[n++] = a3;
    if (a4) argv[n++] = a4;
    if (a5) argv[n++] = a5;
    argv[n] = NULL;
    rc = ART_exec_cmd_result(e, argv, &r);
    if (r) {
        snprintf(out, outsize, "%s%s", ART_result_out(r), ART_result_err(r));
        ART_result_free(r);
    } else {
        snprintf(out, outsize, "%s", ART_engine_error(e));
    }
    return rc;
}

static ART_engine *make(const char *data)
{
    ART_options *o = ART_options_new();
    ART_engine *e = NULL;
    int rc;
    ART_options_set_inprocess(o, data, NULL, NULL);
    ART_options_set_gpu(o, "off");
    e = ART_engine_create(o);
    if (!e) {
        rc = ART_options_error_code(o);
        fprintf(stderr, "cannot create the engine: %s: %s\n", ART_strerror(rc),
                ART_options_error(o));
    }
    ART_options_free(o);
    return e;
}

int main(void)
{
    const char *data = getenv("ART_TEST_DATA_DIR");
    const char *image = getenv("ART_TEST_IMAGE");
    const char *prof = getenv("ART_TEST_PROFILE");
    char dir[] = "/tmp/art-embed-XXXXXX";
    char img[512], arp[512], out[8192], prev[512], p2[512], cmd[600], base[512];
    ART_engine *e1, *e2;
    int rc;

    if (!data || !image) {
        printf("ART_TEST_DATA_DIR / ART_TEST_IMAGE not set: skipped\n");
        return 77;
    }
    if (!ART_has_backend(ART_BACKEND_INPROCESS)) {
        printf("no in-process backend in this build: skipped\n");
        return 77;
    }
    if (!mkdtemp(dir)) {
        return 2;
    }
    snprintf(base, sizeof(base), "%s", strrchr(image, '/') ? strrchr(image, '/') + 1 : image);
    snprintf(img, sizeof(img), "%s/%s", dir, base);
    snprintf(arp, sizeof(arp), "%s/profile.arp", dir);
    snprintf(prev, sizeof(prev), "%s/prev.png", dir);
    snprintf(p2, sizeof(p2), "%s.arp", prof ? "" : image);
    CHECK(copy_file(image, img) == 0);
    if (prof) {
        CHECK(copy_file(prof, arp) == 0);
    } else {
        CHECK(copy_file(p2, arp) == 0);
    }

    e1 = make(data);
    CHECK(e1 != NULL);
    if (!e1) {
        return 1;
    }

    /* errors first: no image open */
    rc = run(e1, out, sizeof(out), "get-info", 0, 0, 0, 0, 0);
    CHECK(rc == ART_ERR_COMMAND);
    CHECK(strstr(out, "ART: error: no image is open in the editor") != NULL);
    rc = run(e1, out, sizeof(out), "quit", 0, 0, 0, 0, 0);
    CHECK(rc == ART_ERR_COMMAND && strstr(out, "quit is not available") != NULL);
    rc = run(e1, out, sizeof(out), "bogus", 0, 0, 0, 0, 0);
    CHECK(rc == ART_ERR_COMMAND && strstr(out, "unknown command: bogus") != NULL);

    CHECK(run(e1, out, sizeof(out), "open", img, 0, 0, 0, 0) == ART_OK);
    CHECK(run(e1, out, sizeof(out), "get-files", 0, 0, 0, 0, 0) == ART_OK);
    CHECK(strstr(out, base) != NULL);
    CHECK(run(e1, out, sizeof(out), "get-info", 0, 0, 0, 0, 0) == ART_OK);
    CHECK(strstr(out, "\"is_raw\":true") != NULL);
    printf("get-info:\n%s", out);

    CHECK(run(e1, out, sizeof(out), "get-history", 0, 0, 0, 0, 0) == ART_OK);
    CHECK(strstr(out, "Photo loaded") != NULL);
    CHECK(run(e1, out, sizeof(out), "apply", arp, 0, 0, 0, 0) == ART_OK);
    CHECK(run(e1, out, sizeof(out), "get-history", 0, 0, 0, 0, 0) == ART_OK);
    CHECK(strstr(out, "\"index\":1,\"selected\":true") != NULL);
    CHECK(run(e1, out, sizeof(out), "undo", 0, 0, 0, 0, 0) == ART_OK);
    CHECK(strstr(out, "{\"ok\":true,\"result\":{\"selected\":0,\"count\":2,") == out);
    CHECK(run(e1, out, sizeof(out), "redo", 0, 0, 0, 0, 0) == ART_OK);
    CHECK(run(e1, out, sizeof(out), "snapshot-add", "S", 0, 0, 0, 0) == ART_OK);
    CHECK(run(e1, out, sizeof(out), "get-snapshots", 0, 0, 0, 0, 0) == ART_OK);
    CHECK(strcmp(out, "{\"ok\":true,\"result\":[{\"name\":\"S\",\"active\":false}]}\n") == 0);
    CHECK(run(e1, out, sizeof(out), "get-spot-wb", "1500", "2000", "20", "20", 0) == ART_OK);
    printf("get-spot-wb:\n%s", out);
    CHECK(strstr(out, "{\"ok\":true,\"result\":{\"temperature\":") == out);
    /* metadata: exiv2 values keep their types */
    CHECK(run(e1, out, sizeof(out), "get-metadata", "--no-makernotes", 0, 0, 0, 0) == ART_OK);
    CHECK(strstr(out, "\"Exif.Image.Make\":\"SONY\"") != NULL);
    CHECK(strstr(out, "\"Exif.Photo.ISOSpeedRatings\":100,") != NULL);
    CHECK(strstr(out, "\"Exif.Photo.FNumber\":2.8,") != NULL);
    CHECK(strstr(out, "ExifTool.MakerNotes") == NULL);
    rc = run(e1, out, sizeof(out), "get-metadata", "--bogus", 0, 0, 0, 0);
    CHECK(rc == ART_ERR_COMMAND && strstr(out, "usage: get-metadata") != NULL);
    CHECK(run(e1, out, sizeof(out), "preview", prev, "--scale", "0.1", 0, 0) == ART_OK);
    printf("preview: %s", out);
    CHECK(access(prev, R_OK) == 0);

    /* export: the real output, full size */
    {
        char xj[512], xt[512];
        struct stat st;
        snprintf(xj, sizeof(xj), "%s/out.jpg", dir);
        snprintf(xt, sizeof(xt), "%s/out.dat", dir);
        CHECK(run(e1, out, sizeof(out), "export", "--list-formats", 0, 0, 0, 0) == ART_OK);
        CHECK(strstr(out, "{\"ok\":true,\"result\":[\"jpg\",\"tif\",\"png\"") == out);
        CHECK(run(e1, out, sizeof(out), "export", xj, "--quality", "50", 0, 0) == ART_OK);
        printf("export: %s", out);
        CHECK(strstr(out, "out.jpg\",\"width\":") != NULL);
        CHECK(stat(xj, &st) == 0 && st.st_size > 10000);
        /* an existing file is not replaced without --overwrite */
        rc = run(e1, out, sizeof(out), "export", xj, 0, 0, 0, 0);
        CHECK(rc == ART_ERR_COMMAND && strstr(out, "already exists") != NULL);
        rc = run(e1, out, sizeof(out), "export", xt, 0, 0, 0, 0);
        CHECK(rc == ART_ERR_COMMAND && strstr(out, "unknown format \"dat\"") != NULL);
        rc = run(e1, out, sizeof(out), "export", xj, "--bits", "16", "--overwrite", 0);
        CHECK(rc == ART_ERR_COMMAND && strstr(out, "--bits is only valid") != NULL);
        rc = run(e1, out, sizeof(out), "export", "/nonexistent-dir/x.jpg", 0, 0, 0, 0);
        CHECK(rc == ART_ERR_COMMAND && strstr(out, "no such directory") != NULL);
        rc = run(e1, out, sizeof(out), "export", img, "--format", "jpg", "--overwrite", 0);
        CHECK(rc == ART_ERR_COMMAND && strstr(out, "cannot overwrite the input") != NULL);
    }
    CHECK(run(e1, out, sizeof(out), "save", 0, 0, 0, 0, 0) == ART_OK);
    CHECK(strstr(out, ".arp") != NULL);

    /* a second engine in the same process: its own image */
    e2 = make(data);
    CHECK(e2 != NULL);
    if (e2) {
        CHECK(run(e2, out, sizeof(out), "get-files", 0, 0, 0, 0, 0) == ART_OK);
        CHECK(strcmp(out, "{\"ok\":true,\"result\":[]}\n") == 0);
        CHECK(run(e2, out, sizeof(out), "get-info", 0, 0, 0, 0, 0) == ART_ERR_COMMAND);
        CHECK(run(e2, out, sizeof(out), "open", img, 0, 0, 0, 0) == ART_OK);
        CHECK(run(e2, out, sizeof(out), "get-snapshots", 0, 0, 0, 0, 0) == ART_OK);
        CHECK(strcmp(out, "{\"ok\":true,\"result\":[{\"name\":\"S\",\"active\":false}]}\n") == 0); /* read from the sidecar saved by e1 */
        ART_engine_destroy(e2);
    }
    CHECK(run(e1, out, sizeof(out), "get-files", 0, 0, 0, 0, 0) == ART_OK);
    CHECK(strstr(out, base) != NULL);
    ART_engine_destroy(e1);

    /* all engines destroyed: the engine of the process is cleaned up; make
     * another one */
    e1 = make(data);
    CHECK(e1 != NULL);
    if (e1) {
        CHECK(run(e1, out, sizeof(out), "open", img, 0, 0, 0, 0) == ART_OK);
        CHECK(run(e1, out, sizeof(out), "get-spot-wb", "1500", "2000", "20", "20", 0) == ART_OK);
        printf("after re-creation:\n%s", out);
        CHECK(strstr(out, "{\"ok\":true,\"result\":{\"temperature\":") == out);
        ART_engine_destroy(e1);
    }

    /* a bad data directory */
    {
        ART_options *o = ART_options_new();
        ART_engine *e = NULL;
        ART_options_set_inprocess(o, "/nonexistent/dir", NULL, NULL);
        /* only meaningful if no engine is alive: the first one decides */
        e = ART_engine_create(o);
        CHECK(e == NULL && ART_options_error_code(o) == ART_ERR_INIT);
        CHECK(strstr(ART_options_error(o), "not a directory") != NULL);
        ART_options_free(o);
    }

    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", dir);
    if (system(cmd) != 0) {
        fprintf(stderr, "could not remove %s\n", dir);
    }
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("libART in-process: all checks passed\n");
    return 0;
}
