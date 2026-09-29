#!/usr/bin/env python3
"""
Builds the Windows release zip: berusky.exe + the game data.

Run it from a "x64 Native Tools Command Prompt for VS" (cl, cmake and ninja
on PATH):

    python tools/package_windows.py [--name SUFFIX] [--build-dir DIR] [--out DIR]

The executable is a GUI application (no console window), with the C/C++
runtime and SDL linked in statically - it needs nothing installed. The zip
(dist/berusky-<version>[-<suffix>]-windows-x64.zip) unpacks into one folder:

    berusky.exe
    data/        GameData, Graphics, Levels, Music, Sound, berusky.ini
    COPYING.txt
    README.txt
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# what the game reads from data/ (the rest is for other platforms / the old build)
DATA_DIRS = ["GameData", "Graphics", "Levels", "Music", "Sound"]
DATA_FILES = ["berusky.ini"]

README = """Berusky {version} - SDL3 port for Windows
==========================================

Berusky is a logic game: guide the five bugs through the levels, collect
five keys and reach the exit. Originally by AnakreoN (1997-2012),
http://www.anakreon.cz/ - GPL v2 (see COPYING.txt).

Run berusky.exe. Nothing has to be installed; keep the data folder next to
the executable.

  berusky.exe                  the game
  berusky.exe -u level.lv3     play a level file
  berusky.exe -e [level.lv3]   the level editor

Keys in a level: arrows move, Shift + arrow moves fast, Tab / 1-5 select a
bug (or click it in the top panel), Ctrl+R restarts the level, F2 / F3 save
and load the position, F1 help, Esc menu, N next music track.

Settings (Settings in the main menu): fullscreen, scaling, filters, sound,
music, their volumes and the language (English / Czech). The sound, music
and Czech texts come from the original DOS version of the game.

Profiles, saved games, your levels and the settings (berusky.ini) are kept
in %APPDATA%\\Anakreon\\Berusky\\.

Source code: https://github.com/missingno7/sdl3_berusky
"""


def project_version():
    with open(os.path.join(ROOT, "CMakeLists.txt"), encoding="utf-8") as f:
        m = re.search(r"project\(berusky VERSION ([0-9.]+)", f.read())
    return m.group(1)


def run(cmd):
    print(">", " ".join(cmd), flush=True)
    subprocess.run(cmd, cwd=ROOT, check=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--name", default="", help="suffix of the package name (e.g. sdl3.1)")
    ap.add_argument("--build-dir", default="build-release")
    ap.add_argument("--out", default="dist")
    args = ap.parse_args()

    if not shutil.which("cl"):
        sys.exit("cl.exe is not on PATH - run this from a x64 Native Tools Command Prompt for VS")

    build = os.path.join(ROOT, args.build_dir)
    run(["cmake", "-S", ".", "-B", build, "-G", "Ninja",
         "-DCMAKE_BUILD_TYPE=Release",
         "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded",
         "-DBERUSKY_WINDOWS_GUI=ON"])
    run(["cmake", "--build", build, "--target", "berusky"])

    version = project_version()
    name = "berusky-" + version + ("-" + args.name if args.name else "") + "-windows-x64"
    out = os.path.join(ROOT, args.out)
    stage = os.path.join(out, name)
    shutil.rmtree(stage, ignore_errors=True)
    os.makedirs(os.path.join(stage, "data"))

    shutil.copy2(os.path.join(build, "berusky.exe"), stage)
    for d in DATA_DIRS:
        shutil.copytree(os.path.join(ROOT, "data", d), os.path.join(stage, "data", d),
                        ignore=shutil.ignore_patterns("Makefile*"))
    for f in DATA_FILES:
        shutil.copy2(os.path.join(ROOT, "data", f), os.path.join(stage, "data"))
    # the GPL text (COPYING at the top of the tree is a dangling automake link)
    shutil.copy2(os.path.join(ROOT, "data", "Windows", "COPYING.TXT"), os.path.join(stage, "COPYING.txt"))
    with open(os.path.join(stage, "README.txt"), "w", encoding="utf-8", newline="\r\n") as f:
        f.write(README.format(version=version))

    zip_path = os.path.join(out, name + ".zip")
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
        for base, _, files in os.walk(stage):
            for f in sorted(files):
                full = os.path.join(base, f)
                z.write(full, os.path.join(name, os.path.relpath(full, stage)))
    print("Package:", zip_path, "(%d bytes)" % os.path.getsize(zip_path))


if __name__ == "__main__":
    main()
