#!/usr/bin/env python3

"""
Helper script to generate an ART self-contained "bundle" directory,
with all the required dependencies (MacOS version)
"""

import os, sys
import shutil
import subprocess
import argparse
from urllib.request import urlopen, Request
import tarfile
import tempfile
import io
import glob
import json
import time
import platform


def getopts():
    p = argparse.ArgumentParser()
    p.add_argument('-o', '--outdir', required=True,
                   help='output directory for the bundle')
    p.add_argument('-e', '--exiftool', action='store_true')
    p.add_argument('-i', '--imageio', help='path to imageio plugins')
    p.add_argument('-b', '--imageio-bin', help='path to imageio binaries')
    p.add_argument('-I', '--imageio-download', action='store_true')
    p.add_argument('-v', '--verbose', action='store_true')
    p.add_argument('-r', '--rpath', action='append')
    p.add_argument('-p', '--prefix')
    p.add_argument('-n', '--no-dmg', action='store_true')
    p.add_argument('-d', '--dmg-name', default='ART')
    p.add_argument('--debug', action='store_true')
    ret = p.parse_args()
    ret.outdir = os.path.join(ret.outdir, 'ART.app')
    return ret


def get_imageio_releases():
    auth = os.getenv('GITHUB_AUTH')
    req = Request('https://api.github.com/repos/artraweditor/ART-imageio/releases')
    if auth is not None:
        req.add_header('authorization', 'Bearer ' + auth)
    with urlopen(req) as f:
        data = f.read().decode('utf-8')
    rel = json.loads(data)
    def key(r):
        return (r['draft'], r['prerelease'],
                time.strptime(r['published_at'], '%Y-%m-%dT%H:%M:%SZ'))
    class RelInfo:
        def __init__(self, rel):
            self.rels = sorted(rel, key=key, reverse=True)
            
        def asset(self, name):
            for rel in self.rels:
                for asset in rel['assets']:
                    if asset['name'] == name:
                        res = Request(asset['browser_download_url'])
                        if auth is not None:
                            res.add_header('authorization', 'Bearer ' + auth)
                        return res
            return None
    return RelInfo(rel)


def getdlls(opts):
    blacklist = ['/System/', '/usr/lib/']
    res = {}
    d = os.path.join(os.getcwd(), 'Contents/MacOS')
    to_process = [os.path.join(d, 'ART'),
                  os.path.join(getprefix(opts), 'bin/dbus-daemon')]
    if opts.verbose:
        print('========== getdlls ==========')
    seen = set()
    while to_process:
        name = to_process[-1]
        to_process.pop()
        if name in seen:
            continue
        seen.add(name)
        if opts.verbose:
            print(f'computing dependencies for: {name}')
        r = subprocess.run(['otool', '-L', name], capture_output=True,
                           encoding='utf-8')
        out = r.stdout
        for line in out.splitlines()[1:]:
            line = line.strip()
            bits = line.split('(compatibility ')
            lib = bits[0].strip()
            if lib.startswith('@rpath/'):
                bn = lib[7:]
                if opts.rpath:
                    for p in opts.rpath:
                        plib = os.path.join(p, bn)
                        if os.path.exists(plib):
                            lib = plib
                            break
            if not any(lib.startswith(p) for p in blacklist):
                key = os.path.basename(lib)
                if key not in res:
                    if opts.verbose:
                        print(f'   {lib}')
                    res[key] = lib
                    to_process.append(lib)
    if opts.verbose:
        print('=============================')
    return sorted(res.values())


def getprefix(opts):
    if opts.prefix:
        return opts.prefix
    d = os.path.join(os.getcwd(), 'Contents/MacOS')
    p = subprocess.Popen(['otool', '-L', os.path.join(d, 'ART')],
                         stdout=subprocess.PIPE)
    out, _ = p.communicate()
    for line in out.decode('utf-8').splitlines()[1:]:
        line = line.strip()
        bits = line.split('(compatibility ')
        lib = bits[0].strip()
        if 'libgtk-3.0' in lib:
            return os.path.dirname(os.path.dirname(lib))
    assert False, "can't determine prefix"


def extra_files(opts):
    pref = getprefix(opts)
    def D(s): return os.path.expanduser(s)
    def P(s): return os.path.join(pref, s)
    if opts.exiftool and os.path.exists('/usr/local/bin/exiftool'):
        extra = [('Contents/Resources/exiftool',
                  [('/usr/local/bin/exiftool', 'exiftool'),
                   ('/usr/local/bin/lib', 'lib')])]
    else:
        extra = []
    imageio = get_imageio_releases() if opts.imageio_download else None
    if opts.imageio:
        extra.append(('Contents/Resources', [(opts.imageio, 'imageio')]))
    elif opts.imageio_download:
        with urlopen(imageio.asset('ART-imageio.tar.gz')) as f:
            if opts.verbose:
                print('downloading ART-imageio.tar.gz '
                      'from GitHub ...')
            tf = tarfile.open(fileobj=io.BytesIO(f.read()))
            if opts.verbose:
                print('unpacking ART-imageio.tar.gz ...')
            tf.extractall(opts.tempdir)
        extra.append(('Contents/Resources',
                      [(os.path.join(opts.tempdir, 'ART-imageio'),
                        'imageio')]))
    if opts.imageio_bin:
        extra.append(('Contents/Resources/imageio',
                      [(opts.imageio_bin, 'bin')]))
    elif opts.imageio_download:
        arch = 'x64' if platform.machine() == 'x86_64' else 'arm64'
        name = f'ART-imageio-bin-macOS-' + arch
        with urlopen(imageio.asset(f'{name}.tar.gz')) as f:
            if opts.verbose:
                print(f'downloading {name}.tar.gz from GitHub ...')
            tf = tarfile.open(fileobj=io.BytesIO(f.read()))
            if opts.verbose:
                print(f'unpacking {name} ...')
            tf.extractall(opts.tempdir)
        extra.append(('Contents/Resources/imageio',
                      [(os.path.join(opts.tempdir, name),
                        'bin')]))
    return [
        ('Contents/Frameworks',
         glob.glob(os.path.join(pref,
                                'lib/gdk-pixbuf-2.0/2.10.0/'
                                'loaders/*.so'))),
        ('Contents/Frameworks',
         glob.glob(os.path.join(pref,
                                'lib/gtk-3.0/3*/immodules/*.so'))),
        ('Contents/Resources', [
            os.path.join(pref, 'bin/gtk-query-immodules-3.0'),
            os.path.join(pref, 'bin/gdk-pixbuf-query-loaders'),
            os.path.join(pref, 'bin/dbus-daemon')
        ]),
        ('Contents/Resources/dbus-1', [
            os.path.join(pref, 'share/dbus-1/session.conf')
        ]),
        ('Contents/Resources/share/icons/Adwaita', [
             P('share/icons/Adwaita/scalable'),
             P('share/icons/Adwaita/index.theme'), 
             P('share/icons/Adwaita/cursors'),
        ]),
        ('Contents/Resources/share/icons', [
             P('share/icons/hicolor'),
        ]),
        ('Contents/Resources/share/glib-2.0/schemas', [
            P('share/glib-2.0/schemas/gschemas.compiled'),
        ]),
        ('Contents/Resources', [
            (D('~/.local/share/lensfun/updates/version_1'), 'lensfun'),
        ]),
        ('Contents/Resources/etc', [
            P('etc/gtk-3.0'),
        ]),
        ('Contents/Resources', [
            P('etc/fonts/fonts.conf'),
        ]),
    ] + extra


def get_version(opts):
    with open('Contents/Resources/AboutThisBuild.txt') as f:
        for line in f:
            if line.startswith('Version: '):
                return line.split()[-1]
    return 'UNKNOWN'


def make_info_plist(opts):
    version = get_version(opts)
    with open(os.path.join(opts.outdir, 'Contents', 'Info.plist'), 'w') as out:
        out.write(f"""\
<?xml version="1.0" encoding="UTF-8"?>
<plist version="1.0">
    <dict>
        <key>CFBundleExecutable</key>
        <string>ART</string>
        <key>CFBundleGetInfoString</key>
        <string>{version}, Copyright © 2004-2010 Gábor Horváth, 2010-2019 RawTherapee Development Team, 2019-2024 Alberto Griggio</string>
        <key>CFBundleIconFile</key>
        <string>ART.icns</string>
        <key>CFBundleIdentifier</key>
        <string>us.pixls.art.ART</string>
        <key>CFBundleInfoDictionaryVersion</key>
        <string>6.0</string>
        <key>CFBundleName</key>
        <string>ART</string>
        <key>CFBundlePackageType</key>
        <string>APPL</string>
        <key>CFBundleShortVersionString</key>
        <string>{version}</string>
        <key>CFBundleSignature</key>
        <string>????</string>
        <key>CFBundleVersion</key>
        <string>{version}</string>
        <key>CFBundleAllowMixedLocalizations</key>
        <true />
        <key>NSHighResolutionCapable</key>
        <true />
        <key>NSHumanReadableCopyright</key>
        <string>Copyright © 2004-2010 Gábor Horváth, 2010-2019 RawTherapee Development Team, 2019-2024 Alberto Griggio</string>
        <key>LSMultipleInstancesProhibited</key>
        <true />
        <key>NSDesktopFolderUsageDescription</key>
        <string>ART requires permission to access the Desktop folder.</string>
        <key>NSDocumentsFolderUsageDescription</key>
        <string>ART requires permission to access the Documents folder.</string>
        <key>NSDownloadsFolderUsageDescription</key>
        <string>ART requires permission to access the Downloads folder.</string>
        <key>NSRemovableVolumesUsageDescription</key>
        <string>ART requires permission to access files on Removable Volumes.</string>        
	<key>CFBundleDocumentTypes</key>
	<array>
		<dict>
			<key>CFBundleTypeRole</key>
			<string>Viewer</string>
			<key>LSItemContentTypes</key>
			<array>
				<string>public.image</string>
				<string>public.directory</string>
			</array>
		</dict>
	</array>
    </dict>
</plist>
""")


def make_icns(opts):
    icondir = os.path.join(opts.tempdir, 'ART.iconset')
    os.mkdir(icondir)
    for i, sz in enumerate([16, 32, 64, 128, 256, 512]):
        shutil.copy2(os.path.join('Contents/Resources/images',
                                  f'ART-logo-{sz}.png'),
                     os.path.join(icondir, f'icon_{sz}x{sz}.png'))
        if i > 0:
            sz2 = sz / 2
            shutil.copy2(os.path.join('Contents/Resources/images',
                                      f'ART-logo-{sz}.png'),
                         os.path.join(icondir, f'icon_{sz2}x{sz2}@2x.png'))
    shutil.copy2(os.path.join('Contents/Resources/images',
                              'ART-logo-1024.png'),
                 os.path.join(icondir, 'icon_512x512@2x.png'))
    subprocess.run(['iconutil', '-c', 'icns', 'ART.iconset'], check=True,
                   cwd=opts.tempdir)
    shutil.copy2(os.path.join(opts.tempdir, 'ART.icns'),
                 os.path.join(opts.outdir, 'Contents/Resources/ART.icns'))
    

def make_dmg(opts):
    if opts.verbose:
        print(f'Creating dmg in {opts.outdir}/{opts.dmg_name}.dmg ...')
    subprocess.run(['hdiutil', 'create', '-format', 'UDBZ',
                    '-fs', 'HFS+', '-srcdir', 'ART.app',
                    '-volname', opts.dmg_name,
                    f'{opts.dmg_name}.dmg'],
                    cwd=os.path.join(opts.outdir, '..'),
                   check=True)

def launcher_build_flags(binary):
    """Return the clang flags needed for the launcher to match the
    architectures and the deployment target of the main binary, so that it
    doesn't end up restricting the bundle."""
    flags = []
    try:
        out = subprocess.run(['lipo', '-archs', binary], check=True,
                             capture_output=True).stdout.decode('utf-8')
        for arch in out.split():
            flags += ['-arch', arch]
    except (subprocess.CalledProcessError, OSError) as e:
        sys.stderr.write(f'WARNING: cannot determine the architectures of '
                         f'{binary} ({e}), using the compiler default\n')
    try:
        out = subprocess.run(['otool', '-l', binary], check=True,
                             capture_output=True).stdout.decode('utf-8')
        # the "minos" entries of the LC_BUILD_VERSION load commands
        vers = [line.split()[1] for line in out.splitlines()
                if line.strip().startswith('minos ')]
        if vers:
            flags.append('-mmacosx-version-min=' +
                         min(vers, key=lambda v: [int(b) for b in v.split('.')]))
    except (subprocess.CalledProcessError, OSError, ValueError) as e:
        sys.stderr.write(f'WARNING: cannot determine the deployment target of '
                         f'{binary} ({e}), using the compiler default\n')
    return flags


def build_launcher(opts, prog):
    src = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       'launcher.c')
    binary = os.path.join(opts.outdir, 'Contents/MacOS', prog)
    dest = os.path.join(opts.outdir, f'Contents/MacOS/{prog}_launch')
    flags = launcher_build_flags(binary)
    if opts.debug:
        flags.append('-DART_LAUNCHER_DEBUG=1')
    if opts.verbose:
        print(f'building launcher for {prog}...')
    subprocess.run(['clang', '-O2', '-Wall'] + flags + [src, '-o', dest],
                   check=True)


def write_launcher_script_cli(opts):
    art_name = 'ART-cli'
    with open(os.path.join(opts.outdir,
                           f'Contents/MacOS/{art_name}'), 'w') as out:
        out.write("""#!/bin/zsh
export ART_restore_GIO_MODULE_DIR=$GIO_MODULE_DIR
export ART_restore_DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH
d="$(/usr/bin/dirname "$(/bin/realpath "$0")")"
d="$(/bin/realpath "${d}/..")"
export DYLD_LIBRARY_PATH="$d/Frameworks"
export ART_EXIFTOOL_BASE_DIR="$d/Resources/exiftool"
exec "$d/MacOS/.ART-cli.bin" "$@"
""")


def main():
    opts = getopts()
    d = os.getcwd()
    if not os.path.exists('Contents/MacOS/ART'):
        sys.stderr.write('ERROR: ART not found! Please run this script '
                         'from the build directory of ART\n')
        sys.exit(1)
    if opts.verbose:
        print('copying %s to %s' % (os.getcwd(), opts.outdir))
    shutil.copytree(d, opts.outdir)
    if not os.path.exists(os.path.join(opts.outdir, 'Contents/Frameworks')):
        os.mkdir(os.path.join(opts.outdir, 'Contents/Frameworks'))
    for lib in getdlls(opts):
        dest = os.path.join(opts.outdir, 'Contents/Frameworks',
                            os.path.basename(lib))
        if os.path.exists(dest):
            if opts.verbose:
                print('SKIPPING already copied: %s' % lib)
        else:
            if opts.verbose:
                print('copying: %s' % lib)
            try:
                shutil.copy2(lib, dest)
                assert os.path.exists(dest)
            except FileNotFoundError as e:
                sys.stderr.write(f'WARNING: {lib} not found, skipping\n')
    with tempfile.TemporaryDirectory() as d:
        opts.tempdir = d
        for key, elems in extra_files(opts):
            for elem in elems:
                name = None
                if isinstance(elem, tuple):
                    elem, name = elem
                else:
                    name = os.path.basename(elem)
                if opts.verbose:
                    print('copying: %s' % elem)
                if not os.path.exists(elem):
                    print('SKIPPING non-existing: %s' % elem)
                elif os.path.isdir(elem):
                    shutil.copytree(elem, os.path.join(opts.outdir, key, name))
                else:
                    dest = os.path.join(opts.outdir, key, name)
                    destdir = os.path.dirname(dest)
                    if not os.path.exists(destdir):
                        os.makedirs(destdir)
                    shutil.copy2(elem, dest)
        make_info_plist(opts)
        make_icns(opts)

        build_launcher(opts, 'ART')

    os.makedirs(os.path.join(opts.outdir, 'Contents/Resources/share/gtk-3.0'))
    with open(os.path.join(opts.outdir,
                           'Contents/Resources/share/gtk-3.0/settings.ini'),
              'w') as out:
        out.write('[Settings]\ngtk-primary-button-warps-slider = true\n'
                  'gtk-overlay-scrolling = true\n')
    with open(os.path.join(opts.outdir, 'Contents/Resources/options'),
              'a') as out:
        out.write('\n[Lensfun]\nDBDirectory=lensfun\n')
    for name in ('ART', 'ART-cli'):
        shutil.move(os.path.join(opts.outdir, 'Contents/MacOS', name),
                    os.path.join(opts.outdir, 'Contents/MacOS',
                                 '.' + name + '.bin'))
    # the launcher takes the place of the GUI binary: it is what
    # LaunchServices starts, and it execv's .ART.bin (see tools/osx/launcher.c)
    shutil.move(os.path.join(opts.outdir, 'Contents/MacOS', 'ART_launch'),
                os.path.join(opts.outdir, 'Contents/MacOS', 'ART'))
    write_launcher_script_cli(opts)
    for name in ('ART', 'ART-cli'):
        os.chmod(os.path.join(opts.outdir, 'Contents/MacOS', name), 0o755)
    if not opts.no_dmg:
        make_dmg(opts)

if __name__ == '__main__':
    main()
