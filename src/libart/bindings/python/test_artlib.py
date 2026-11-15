"""Smoke test of the Python binding against a real server or the in-process
engine. Run with:

    ART_LIBRARY=.../libART.dylib python3 test_artlib.py remote   # a server is running
    ART_LIBRARY=... ART_TEST_DATA_DIR=... ART_TEST_IMAGE=... python3 test_artlib.py inprocess

(`remote` needs ART_TEST_IMAGE too; the server's socket is the default one or
ART_SERVER_SOCKET.) Edits a copy of the image.
"""
import os
import shutil
import sys
import tempfile

import artlib
from artlib import Engine, ArtError


def main(mode):
    image = os.environ["ART_TEST_IMAGE"]
    work = tempfile.mkdtemp(prefix="art-py-")
    img = os.path.join(work, os.path.basename(image))
    shutil.copy(image, img)
    prof = os.environ.get("ART_TEST_PROFILE", image + ".arp")
    kw = {}
    if mode == "inprocess":
        kw = dict(inprocess=True, data_dir=os.environ["ART_TEST_DATA_DIR"], gpu="off")
    chunks = []
    with Engine(on_output=chunks.append, **kw) as art:
        assert art.run("ping").out.strip() == "pong"
        assert chunks == ["pong\n"], chunks      # the streaming callback
        art.run("open", img, check=True)
        info = art.run("get-info", check=True).out
        assert "is_raw: true" in info, info
        r = art.run("get-spot-wb", 1500, 2000, 20, 20, check=True)
        assert r.out.startswith("temperature: 3735.61"), r.out
        r = art.run("get-spot-wb", 0, 0, 0, 5)           # fails: empty rectangle
        assert not r.ok and r.code == 1 and "must not be empty" in r.err, r
        try:
            art.run("get-spot-wb", 0, 0, 0, 5, check=True)
            raise AssertionError("no exception")
        except artlib.CommandError as e:
            assert "must not be empty" in str(e)
        art.run("apply", prof, check=True)
        hist = art.run("get-history", check=True).out
        assert hist.count("\n") == 2, hist
        art.run("snapshot-add", "name with spaces", check=True)
        assert "name with spaces" in art.run("get-snapshots").out
        prev = os.path.join(work, "p.png")
        assert "(312x468)" in art.run("preview", prev, "--scale", 0.1, check=True).out
        assert os.path.getsize(prev) > 1000
        out = os.path.join(work, "out.jpg")
        r = art.run("export", out, "--quality", 50, check=True)
        assert r.out.strip().endswith("out.jpg (3124x4684)"), r.out
        assert os.path.getsize(out) > 10000
        r = art.run("export", out)                      # exists: not replaced
        assert not r.ok and "already exists" in r.err, r
        r = art.run("export", out, "--bits", 16, "--overwrite")
        assert not r.ok and "--bits is only valid" in r.err, r
        assert art.run("export", "--list-formats", check=True).out.startswith("jpg\ntif\npng\n")
    print("python binding (%s): ok" % mode)

    # library-level failures are exceptions
    if mode == "remote":
        try:
            with Engine(socket=os.path.join(work, "none.sock")) as art:
                art.run("ping")
            raise AssertionError("no exception")
        except ArtError as e:
            assert e.code == 3 and "no running server" in e.message, e
        try:
            Engine(socket=os.path.join(work, "x"), library="/nonexistent/lib")
        except OSError:
            pass
    shutil.rmtree(work)


main(sys.argv[1] if len(sys.argv) > 1 else "remote")
