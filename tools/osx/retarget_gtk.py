#!/usr/bin/env python3
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

"""Make sure that only one GTK is loaded (macOS build trees).

CMake links ART against the first libgtk-3/libgdk-3 of the link directories
(say, a self-built GTK in /opt/local), but the C++ bindings and gtk-mac-
integration it also links (say, from Homebrew) record the absolute path of the
GTK they were built against: dyld then loads two GTKs, whose types clash
("cannot register existing type 'GdkCursor'", a hang at start-up).

This script makes private copies of those libraries in which

  * the references to libgtk-3/libgdk-3 point at the GTK the linker picks,
  * their compatibility version is the one of that GTK (dyld refuses a library
    older than the one a client was built against, and the GTK that was
    chosen is allowed to be older),
  * references among the copies point at the copies,

and re-signs them (ad hoc: patching invalidates the signature, and arm64
refuses to load them otherwise). ART is then linked against the copies by
putting their directory first in the link path (cmake/modules/ARTRetargetGtk.cmake).

    retarget_gtk.py --libdirs DIR:DIR:... --list
        print what needs copying (nothing: all libraries already agree) and,
        on stderr, the GTK that was picked
    retarget_gtk.py --libdirs DIR:DIR:... --build OUTDIR
        make the copies (and OUTDIR/.stamp)
"""

import argparse
import os
import shutil
import struct
import subprocess
import sys

# the libraries that link GTK and that ART links: by the name given to -l
SEEDS = ["gtkmm-3.0", "gdkmm-3.0", "gtkmacintegration-gtk3"]
# what is being unified: <leaf of the install name> of the GTK libraries
GTK_NAMES = ["gtk-3", "gdk-3"]

MH_MAGIC_64 = 0xFEEDFACF
FAT_MAGIC = 0xCAFEBABE
LC_ID_DYLIB = 0xD
LC_LOAD_DYLIB = 0xC
LC_LOAD_WEAK_DYLIB = 0x80000018
LC_REEXPORT_DYLIB = 0x8000001F
LOAD_COMMANDS = (LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, LC_REEXPORT_DYLIB)


class Error(Exception):
    pass


# ---- Mach-O: just the dylib load commands ------------------------------------

def slices(data):
    """The offsets of the (64-bit, little endian) images of a file."""
    magic = struct.unpack_from(">I", data, 0)[0]
    if magic == FAT_MAGIC:
        n = struct.unpack_from(">I", data, 4)[0]
        out = []
        for i in range(n):
            cputype, cpusub, off, size, align = struct.unpack_from(
                ">iiIII", data, 8 + 20 * i)
            out.append(off)
        return out
    return [0]


def dylib_commands(data, base):
    """(cmd, offset, name, current, compat) of the dylib commands of the image
    at `base`."""
    magic = struct.unpack_from("<I", data, base)[0]
    if magic != MH_MAGIC_64:
        raise Error("not a 64-bit Mach-O image")
    ncmds = struct.unpack_from("<I", data, base + 16)[0]
    off = base + 32
    for _ in range(ncmds):
        cmd, size = struct.unpack_from("<II", data, off)
        if cmd == LC_ID_DYLIB or cmd in LOAD_COMMANDS:
            nameoff, _ts, cur, compat = struct.unpack_from("<IIII", data, off + 8)
            end = data.index(b"\0", off + nameoff)
            name = bytes(data[off + nameoff:end]).decode()
            yield cmd, off, name, cur, compat
        off += size


def read_dylib(path):
    """(install name, current, compat, [dependencies]) of a dylib."""
    with open(path, "rb") as f:
        data = f.read()
    ident = None
    deps = []
    for cmd, off, name, cur, compat in dylib_commands(data, slices(data)[0]):
        if cmd == LC_ID_DYLIB:
            ident = (name, cur, compat)
        else:
            deps.append(name)
    if ident is None:
        raise Error("%s has no install name" % path)
    return ident[0], ident[1], ident[2], deps


def set_compat(path, name, compat):
    """Set the compatibility version recorded for the dependency `name`."""
    with open(path, "rb") as f:
        data = bytearray(f.read())
    for base in slices(data):
        for cmd, off, n, cur, comp in list(dylib_commands(data, base)):
            if cmd in LOAD_COMMANDS and n == name:
                struct.pack_into("<I", data, off + 8 + 12, compat)
    with open(path, "wb") as f:
        f.write(data)


# ---- what to do -----------------------------------------------------------------

def find_lib(name, libdirs):
    """What the linker finds for -l<name>: the first directory that has it."""
    for d in libdirs:
        p = os.path.join(d, "lib%s.dylib" % name)
        if os.path.exists(p):
            return os.path.realpath(p)
    return None


def leaf(path):
    """libgtk-3.0.dylib -> gtk-3 (the part the GTK libraries are told by)"""
    b = os.path.basename(path)
    if not (b.startswith("lib") and b.endswith(".dylib")):
        return None
    return b[3:-6].split(".")[0]


class Plan:
    def __init__(self, libdirs):
        self.chosen = {}   # leaf -> (real path, install name, current, compat)
        self.todo = []     # [(link name, real path, install name)]
        for n in GTK_NAMES:
            real = find_lib(n, libdirs)
            if real is None:
                raise Error("no lib%s.dylib in the link directories" % n)
            ident, cur, compat, _ = read_dylib(real)
            self.chosen[n] = (real, ident, cur, compat)
        for name in SEEDS:
            real = find_lib(name, libdirs)
            if real is None:
                continue  # not used by this build
            ident, _, _, deps = read_dylib(real)
            if self.foreign(deps):
                self.todo.append((name, real, ident))

    def foreign(self, deps):
        """The dependencies that are a GTK library, but not the chosen one."""
        out = []
        for d in deps:
            n = leaf(d)
            if n in self.chosen and d != self.chosen[n][1] and \
                    os.path.realpath(d) != self.chosen[n][0]:
                out.append(d)
        return out


def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise Error("%s failed: %s" % (" ".join(cmd[:2]), r.stderr.strip()))


def build(plan, outdir):
    os.makedirs(outdir, exist_ok=True)
    # the copies, by the real path of the library they replace: a dependency
    # may be spelled by its install name or by the path it was linked from
    copies = {}
    for name, real, ident in plan.todo:
        copies[real] = os.path.join(outdir, os.path.basename(real))
    for name, real, ident in plan.todo:
        dst = copies[real]
        if os.path.lexists(dst):
            os.unlink(dst)
        shutil.copyfile(real, dst)
        os.chmod(dst, 0o755)
        _, _, _, deps = read_dylib(real)
        args = ["install_name_tool", "-id", dst]
        for d in plan.foreign(deps):
            args += ["-change", d, plan.chosen[leaf(d)][1]]
        for d in deps:
            r = os.path.realpath(d)
            if r in copies and r != real:
                args += ["-change", d, copies[r]]
        run(args + [dst])
        for d in plan.foreign(deps):
            _, newid, _, compat = plan.chosen[leaf(d)]
            set_compat(dst, newid, compat)
        # a signature that does not match is fatal on arm64
        if shutil.which("codesign"):
            run(["codesign", "--force", "--sign", "-", dst])
        link = os.path.join(outdir, "lib%s.dylib" % name)
        if os.path.lexists(link):
            os.unlink(link)
        os.symlink(os.path.basename(dst), link)
    with open(os.path.join(outdir, ".stamp"), "w") as f:
        f.write("\n".join(r for _, r, _ in plan.todo) + "\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--libdirs", required=True,
                    help="the link directories, in order, separated by ':'")
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--list", action="store_true")
    g.add_argument("--build", metavar="OUTDIR")
    a = ap.parse_args()
    libdirs = [d for d in a.libdirs.split(":") if d]
    try:
        plan = Plan(libdirs)
        if a.list:
            print("GTK: %s" % plan.chosen["gtk-3"][0], file=sys.stderr)
            for _, real, _ in plan.todo:
                print(real)
        else:
            build(plan, a.build)
    except (Error, OSError, ValueError, struct.error) as e:
        print("retarget_gtk.py: %s" % e, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
