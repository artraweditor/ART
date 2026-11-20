#
#  This file is part of ART.
#
#  ART is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  ART is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with ART.  If not, see <http://www.gnu.org/licenses/>.

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
        assert art.run("ping").data == "pong"
        assert chunks[0] == '{"ok":true,"result":"pong"}\n', chunks    # the streaming callback
        art.run("open", img, check=True)
        info = art.run("get-info", check=True).data
        assert info["is_raw"] is True and info["full_width"] == 3124, info
        r = art.run("get-spot-wb", 1500, 2000, 20, 20, check=True)
        assert abs(r.data["temperature"] - 3735.61) < 0.01, r.data
        assert r.data["green_multiplier"] == 1, r.data
        r = art.run("get-spot-wb", 0, 0, 0, 5)           # fails: empty rectangle
        assert not r.ok and r.code == 1 and "must not be empty" in r.err, r
        assert r.reply == {"ok": False, "error": r.error} and "must not be empty" in r.error, r
        try:
            art.run("get-spot-wb", 0, 0, 0, 5, check=True)
            raise AssertionError("no exception")
        except artlib.CommandError as e:
            assert "must not be empty" in str(e)
        md = art.run("get-metadata", "--no-makernotes", check=True).data
        assert md["Exif.Image.Make"] == "SONY" and md["Exif.Photo.ISOSpeedRatings"] == 100, md
        assert isinstance(md["Exif.Photo.FNumber"], float) and isinstance(md["Exif.Photo.ComponentsConfiguration"], list)
        assert not any(k.startswith("ExifTool.") for k in md)
        mn = art.run("get-metadata", "--makernotes", check=True).data
        assert any(k.startswith("ExifTool.MakerNotes.") for k in mn), "no makernotes (exiftool missing?)"
        assert all(isinstance(v, str) for k, v in mn.items() if k.startswith("ExifTool."))
        art.run("apply", prof, check=True)
        edit_arp = os.path.join(work, "ed.arp")
        with open(edit_arp, "w") as f:
            f.write("[Exif]\nArtist=Jane Tester\n")
        art.run("apply", edit_arp, check=True)
        assert art.run("get-metadata", "--no-makernotes", check=True).data["Exif.Image.Artist"] == "Jane Tester"
        assert art.run("get-metadata", "--no-makernotes", "--edits", check=True).data["Exif.Image.Artist"] == "Jane Tester"
        assert "Exif.Image.Artist" not in art.run("get-metadata", "--no-makernotes", "--no-edits", check=True).data
        r = art.run("get-metadata", "--edits", "--no-edits")
        assert not r.ok and "usage: get-metadata" in r.error, r
        art.run("undo", check=True)
        # the special profile names
        def exposure():
            txt = art.run("get-params", check=True).data["params"]
            return float(txt.split("[Exposure]")[1].split("Compensation=")[1].split()[0])
        assert exposure() != 0
        art.run("apply", ":neutral", check=True)
        assert exposure() == 0
        art.run("apply", prof, check=True)
        assert exposure() != 0
        art.run("apply", ":default", check=True)
        r = art.run("apply", "--append", ":neutral")
        assert not r.ok and "--append cannot be used" in r.error, r
        art.run("undo", 3, check=True)
        hist = art.run("get-history", check=True).data
        assert [e["text"] for e in hist["entries"]][0] == "Photo loaded" and hist["selected"] == 1, hist
        art.run("snapshot-add", "name with spaces", check=True)
        assert art.run("get-snapshots").data == [{"name": "name with spaces", "active": False}]
        prev = os.path.join(work, "p.png")
        assert art.run("preview", prev, "--scale", 0.1, check=True).data == \
            {"file": prev, "width": 312, "height": 468}
        assert os.path.getsize(prev) > 1000
        out = os.path.join(work, "out.jpg")
        r = art.run("export", out, "--quality", 50, check=True)
        assert r.data == {"file": out, "width": 3124, "height": 4684, "format": "jpg"}, r.data
        assert os.path.getsize(out) > 10000
        r = art.run("export", out)                      # exists: not replaced
        assert not r.ok and "already exists" in r.err, r
        r = art.run("export", out, "--bits", 16, "--overwrite")
        assert not r.ok and "--bits is only valid" in r.err, r
        assert art.run("export", "--list-formats", check=True).data[:3] == ["jpg", "tif", "png"]
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
