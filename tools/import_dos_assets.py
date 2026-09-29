#!/usr/bin/env python3
"""
Import the runtime assets that the SDL3 port takes from the DOS original.

    python tools/import_dos_assets.py [--original original] [--check]

The DOS release (the `original/` reference directory, not needed at run
time) has the music, the sound effects and the Czech texts that the later
1.x code base lost. This script copies / converts them into `data/`:

  original/game/MOD/X_0nn.XM            -> data/Music/x_0nn.xm      (copied)
  original/game/SMP/SMP_0nn.RAW         -> data/Sound/smp_0nn.raw   (copied)
  original/src/BERUSKY/DOCLEP/TEXTY.TXT -> data/GameData/cs/hints.dat
  original/src/BERUSKY/DOCLEP/EPIZODAn.TXT -> data/GameData/cs/end(n-1).dat
  original/src/BERUSKY/DOCLEP/CREDIT.TXB   -> data/GameData/cs/credits.dat

Music and samples are copied byte for byte (only the file names are lower
case, the DOS game opened them as "mod\\x_%.3d.xm" / "smp\\smp_%.3d.raw").
The texts are converted from CP852 (the .TXT sources of the shipped .TXB
files) or from the game's own accent markup (CREDIT.TXB) to UTF-8 with CRLF
line ends, like the other files of data/GameData.

--check compares data/ with what the script would write and fails when they
differ, the files in original/ are never modified.
"""

import argparse
import hashlib
import os
import sys
import unicodedata

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MUSIC_TRACKS = 30
SAMPLES = 20

# The accent markup of the DOS texts (made by the L2BER tool): the marker
# is written before the letter it belongs to.
MARKUP = {'$': '́',   # carka   - acute
          '^': '̌',   # hacek   - caron
          '#': '̊'}   # krouzek - ring

# What the credits of the later versions (data/GameData/credits.dat) add to
# the DOS credits - new text, the DOS game doesn't have it.
CREDITS_PORT = """



Berušky (C) AnakreoN 2007
šířeno pod licencí GPL

překlad do angličtiny
Radek Biba

poděkování také
Michal Šimoník

anakreon@anakreon.cz
www.anakreon.cz
bližší informace v souboru COPYING
"""


def markup_decode(text):
    out = []
    i = 0
    while i < len(text):
        c = text[i]
        if c in MARKUP and i + 1 < len(text) and text[i + 1].isalpha():
            out.append(unicodedata.normalize('NFC', text[i + 1] + MARKUP[c]))
            i += 2
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def crlf(text):
    text = text.replace('\r\n', '\n').replace('\r', '\n')
    return text.replace('\n', '\r\n').encode('utf-8')


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    with open(path, 'rb') as f:
        return f.read()


def planned_files(orig):
    """{relative path in data/: bytes}"""
    files = {}
    doclep = os.path.join(orig, 'src', 'BERUSKY', 'DOCLEP')

    music_list = []
    for i in range(MUSIC_TRACKS):
        src = os.path.join(orig, 'game', 'MOD', 'X_%03d.XM' % i)
        data = read(src)
        files['Music/x_%03d.xm' % i] = data
        music_list.append('x_%03d.xm  %8d  %s  game/MOD/X_%03d.XM' % (i, len(data), sha256(data), i))

    sample_list = []
    for i in range(SAMPLES):
        src = os.path.join(orig, 'game', 'SMP', 'SMP_%03d.RAW' % i)
        data = read(src)
        files['Sound/smp_%03d.raw' % i] = data
        sample_list.append('smp_%03d.raw  %6d  %s  game/SMP/SMP_%03d.RAW' % (i, len(data), sha256(data), i))

    files['Music/SOURCE.txt'] = crlf(
        "Music of Berusky (DOS, 1999) - 30 FastTracker II modules.\n"
        "Copied unchanged from the DOS release (original/game/MOD) by\n"
        "tools/import_dos_assets.py. The DOS release is under the GNU GPL\n"
        "(see COPYING). Authors: Martin \"Linda\" Linda, Roman \"Robe\" Bednarik.\n"
        "Which track plays when: docs/AUDIO.md\n\n"
        "file        bytes     sha256                                                            source\n"
        + '\n'.join(music_list) + '\n')
    files['Sound/SOURCE.txt'] = crlf(
        "Sound effects of Berusky (DOS, 1999) - 20 raw samples,\n"
        "16-bit signed little endian mono, played at 20050 Hz, not looped.\n"
        "Copied unchanged from the DOS release (original/game/SMP) by\n"
        "tools/import_dos_assets.py. The DOS release is under the GNU GPL\n"
        "(see COPYING). Samples 1, 9 and 19 were never used by the DOS game.\n"
        "Which sound plays when: docs/AUDIO.md\n\n"
        "file          bytes   sha256                                                            source\n"
        + '\n'.join(sample_list) + '\n')

    texty = read(os.path.join(doclep, 'TEXTY.TXT')).decode('cp852')
    files['GameData/cs/hints.dat'] = crlf(texty)

    for n in range(1, 6):
        ep = read(os.path.join(doclep, 'EPIZODA%d.TXT' % n)).decode('cp852')
        files['GameData/cs/end%d.dat' % (n - 1)] = crlf(ep)

    # The DOS scroller started the text at the bottom of its window (23 empty
    # lines first), the port's one starts at the top: 4 empty lines like
    # data/GameData/credits.dat
    credit = markup_decode(read(os.path.join(doclep, 'CREDIT.TXB')).decode('cp852'))
    credit = credit.replace('\r\n', '\n').strip('\n')
    files['GameData/cs/credits.dat'] = crlf('\n' * 4 + credit + '\n' + CREDITS_PORT)

    return files


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--original', default=os.path.join(ROOT, 'original'),
                    help='the DOS reference directory (default: original/)')
    ap.add_argument('--data', default=os.path.join(ROOT, 'data'), help='the game data directory')
    ap.add_argument('--check', action='store_true', help='only verify data/, write nothing')
    args = ap.parse_args()

    if not os.path.isdir(args.original):
        sys.exit('%s not found - the DOS reference files are needed' % args.original)

    files = planned_files(args.original)
    bad = 0
    for rel, data in sorted(files.items()):
        dst = os.path.join(args.data, *rel.split('/'))
        current = read(dst) if os.path.exists(dst) else None
        if current == data:
            continue
        # Texts may be checked out with LF line ends (git normalizes them)
        if current is not None and rel.endswith(('.dat', '.txt')) and \
           current.replace(b'\r\n', b'\n') == data.replace(b'\r\n', b'\n'):
            continue
        if args.check:
            print('differs: %s' % rel)
            bad += 1
            continue
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        with open(dst, 'wb') as f:
            f.write(data)
        print('wrote %s' % rel)

    if args.check:
        print('%d files checked, %d differ' % (len(files), bad))
        sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
