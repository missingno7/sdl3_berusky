#!/usr/bin/env python3
"""
Regression tests for the SDL3 port: scripted sessions + framebuffer screenshots.

Every tests/scripts/NN_name.txt is a script for the game (see src/test_script.h).
The game runs with an empty user directory, replays the script and saves the
software framebuffer (`shot file.bmp`) at the given moments.
Screenshots are compared with tests/expected/NN_name.sha256 (hashes of the raw
pixels) - the game is deterministic in the test mode (fixed random seed, 30 Hz
ticks are counted, not timed).

    python tests/run_tests.py [--exe build/berusky] [--update] [--keep] [name ...]

  --update   write the current hashes as the expected ones
  --keep     keep the output (BMP, log) in build/tests/<name>/ (always kept on failure)
"""

import argparse
import hashlib
import os
import shutil
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)


def find_exe(explicit):
    if explicit:
        return explicit
    for cand in ("build/berusky.exe", "build/berusky", "build/Release/berusky.exe"):
        p = os.path.join(ROOT, cand)
        if os.path.exists(p):
            return p
    sys.exit("berusky executable not found, use --exe")


def pixel_hash(path):
    """SHA-256 of the BMP pixel data (so the hash doesn't depend on the file header)."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:2] != b"BM":
        return hashlib.sha256(data).hexdigest()
    offset = struct.unpack_from("<I", data, 10)[0]
    return hashlib.sha256(data[offset:]).hexdigest()


def script_args(script):
    """A script may start with '# args: -e level.lv3' - command line for the game."""
    with open(script) as f:
        first = f.readline().strip()
    if first.startswith("# args:"):
        return first[len("# args:"):].split()
    return []


def run_script(exe, script, out_dir):
    # (the directory itself may be locked on Windows when it's somebody's cwd)
    if os.path.isdir(out_dir):
        for entry in os.listdir(out_dir):
            path = os.path.join(out_dir, entry)
            if os.path.isdir(path):
                shutil.rmtree(path, ignore_errors=True)
            else:
                os.remove(path)
    os.makedirs(out_dir, exist_ok=True)
    env = dict(os.environ)
    env["BERUSKY_USER_DIR"] = os.path.join(out_dir, "user")
    env["BERUSKY_TEST_SCRIPT"] = script
    env["BERUSKY_TEST_OUT"] = out_dir
    with open(os.path.join(out_dir, "log.txt"), "w") as log:
        try:
            r = subprocess.run([exe] + script_args(script), env=env, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, timeout=120)
            return r.returncode
        except subprocess.TimeoutExpired:
            return "timeout"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe")
    ap.add_argument("--update", action="store_true")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("names", nargs="*")
    args = ap.parse_args()

    exe = find_exe(args.exe)
    scripts_dir = os.path.join(HERE, "scripts")
    expected_dir = os.path.join(HERE, "expected")
    os.makedirs(expected_dir, exist_ok=True)

    names = args.names or sorted(f[:-4] for f in os.listdir(scripts_dir) if f.endswith(".txt"))
    failed = 0

    for name in names:
        script = os.path.join(scripts_dir, name + ".txt")
        out_dir = os.path.join(ROOT, "build", "tests", name)
        code = run_script(exe, script, out_dir)

        shots = sorted(f for f in os.listdir(out_dir) if f.endswith(".bmp"))
        hashes = {s: pixel_hash(os.path.join(out_dir, s)) for s in shots}
        expected_file = os.path.join(expected_dir, name + ".sha256")

        if args.update:
            with open(expected_file, "w") as f:
                for s in shots:
                    f.write("%s %s\n" % (hashes[s], s))
            print("UPDATED  %-28s %d screenshots" % (name, len(shots)))
            continue

        expected = {}
        if os.path.exists(expected_file):
            for line in open(expected_file):
                if line.strip():
                    h, s = line.split()
                    expected[s] = h

        problems = []
        if code != 0:
            problems.append("exit code %s" % code)
        for s in sorted(set(expected) | set(hashes)):
            if s not in hashes:
                problems.append("missing screenshot %s" % s)
            elif s not in expected:
                problems.append("no expected hash for %s" % s)
            elif hashes[s] != expected[s]:
                problems.append("differs: %s" % s)

        if problems:
            failed += 1
            print("FAIL     %-28s %s (output kept in %s)" % (name, "; ".join(problems), out_dir))
        else:
            print("ok       %-28s %d screenshots" % (name, len(shots)))
            if not args.keep:
                shutil.rmtree(out_dir, ignore_errors=True)

    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
