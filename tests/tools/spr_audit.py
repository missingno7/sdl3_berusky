#!/usr/bin/env python3
"""
Audit of the sprite sheet metadata (data/Graphics/*.spr).

The .spr format (see sprite_store::sprite_insert in src/2d_graph.cpp):

    ; comment
    s x y w h [scale]     one sprite, rectangle in pixels of the PNG
    f dx dy dw dh count   'count' more sprites, each one shifted by (dx, dy, dw, dh)
    m density N           (new, optional) native pixel density of the sheet
    m cell 0|1            (new, optional) level-cell art (1) or UI art (0)

The optional 5th field of 's' is the legacy double-size flag of Berusky 1.7:

    scale = 1   the sheet is original 1x artwork; the old double-size mode
                enlarged it 2x at load time (surface::scale)
    scale = 0   the sheet was drawn for the double-size (1280x900) mode:
                its pixels are genuine 2x artwork
    (missing)   the sheet was drawn unscaled in both modes: UI artwork (fonts,
                logos, panel icons) at 1x

The modern renderer derives the per-sheet native density from that:
scale 1 / missing -> density 1, scale 0 -> density 2, unless 'm density' says
otherwise. Sheets with a legacy scale field live in the level-cell space
(they follow the cell zoom of the editor), others are UI art.

    python tests/tools/spr_audit.py [--graphics data/Graphics] [--markdown]
"""

import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))


def png_size(path):
    with open(path, "rb") as f:
        head = f.read(24)
    if head[:8] != b"\x89PNG\r\n\x1a\n":
        return None
    return struct.unpack(">II", head[16:24])


def parse_spr(path):
    sprites = []           # (x, y, w, h, scale or None)
    meta = {}
    rec = [0, 0, 0, 0]
    scale = None
    with open(path) as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith(";"):
                continue
            parts = line.split()
            if parts[0] == "s":
                nums = [int(v) for v in parts[1:6]]
                rec = nums[:4]
                scale = nums[4] if len(nums) > 4 else None
                sprites.append(tuple(rec) + (scale,))
            elif parts[0] == "f":
                dx, dy, dw, dh, count = [int(v) for v in parts[1:6]]
                for _ in range(count):
                    rec = [rec[0] + dx, rec[1] + dy, rec[2] + dw, rec[3] + dh]
                    sprites.append(tuple(rec) + (scale,))
            elif parts[0] == "m" and len(parts) >= 3:
                meta[parts[1]] = parts[2]
    return sprites, meta


def audit(graphics_dir):
    rows = []
    for name in sorted(os.listdir(graphics_dir)):
        if not name.endswith(".spr"):
            continue
        spr = os.path.join(graphics_dir, name)
        png = spr[:-4] + ".png"
        size = png_size(png) if os.path.exists(png) else None
        sprites, meta = parse_spr(spr)
        flags = sorted({s[4] for s in sprites}, key=lambda v: -1 if v is None else v)

        problems = []
        if len(flags) > 1:
            problems.append("mixed scale fields %s" % flags)
        legacy = flags[0] if flags else None

        if "density" in meta:
            density = float(meta["density"])
            origin = "m density"
        elif legacy == 0:
            density, origin = 2.0, "scale 0"
        else:
            density, origin = 1.0, ("scale 1" if legacy == 1 else "no scale")
        cell = meta.get("cell")
        cell = (cell == "1") if cell is not None else legacy is not None

        for s in sprites:
            x, y, w, h = s[:4]
            if size and (x + w > size[0] or y + h > size[1]):
                problems.append("sprite %s outside the PNG" % (s[:4],))
                break
            if any(v % density for v in (x, y, w, h)):
                problems.append("sprite %s not on the %gx pixel grid" % (s[:4], density))
                break

        # Nothing is wider than the reference composition (640x480 logical
        # units) - a bigger sheet has a wrong (missing) density
        if sprites and max(s[2] for s in sprites) / density > 640:
            problems.append("wider than the 640 unit composition at %gx: missing density?" % density)

        first = sprites[0] if sprites else (0, 0, 0, 0, None)
        rows.append({
            "sheet": name,
            "png": "%dx%d" % size if size else "missing",
            "sprites": len(sprites),
            "cell_px": "%dx%d" % (first[2], first[3]),
            "legacy": "-" if legacy is None else str(legacy),
            "density": density,
            "origin": origin,
            "logical": "%gx%g" % (first[2] / density, first[3] / density),
            "space": "cell" if cell else "ui",
            "problems": "; ".join(problems),
        })
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--graphics", default=os.path.join(ROOT, "data", "Graphics"))
    ap.add_argument("--markdown", action="store_true")
    args = ap.parse_args()

    rows = audit(args.graphics)
    cols = ["sheet", "png", "sprites", "cell_px", "legacy", "density", "origin", "logical", "space", "problems"]
    if args.markdown:
        print("| " + " | ".join(cols) + " |")
        print("|" + "---|" * len(cols))
        for r in rows:
            print("| " + " | ".join(str(r[c]) for c in cols) + " |")
    else:
        widths = {c: max(len(c), max(len(str(r[c])) for r in rows)) for c in cols}
        print("  ".join(c.ljust(widths[c]) for c in cols))
        for r in rows:
            print("  ".join(str(r[c]).ljust(widths[c]) for c in cols))

    summary = {}
    for r in rows:
        summary.setdefault((r["density"], r["space"]), []).append(r["sheet"])
    print()
    for (density, space), sheets in sorted(summary.items()):
        print("density %gx, %s art: %d sheets" % (density, space, len(sheets)))
    bad = [r for r in rows if r["problems"]]
    for r in bad:
        print("PROBLEM %s: %s" % (r["sheet"], r["problems"]))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
