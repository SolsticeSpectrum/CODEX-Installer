from __future__ import annotations
import argparse
import json
import sys
from pathlib import Path

from . import reader, slicer
from .utils import theme


def _strip_bitmaps(result: dict) -> dict:
    out = {}
    for key, val in result.items():
        if key == "bitmaps":
            out[key] = [{k: v for k, v in bmp.items() if k != "image"} for bmp in val]
        elif key == "objects":
            out[key] = _strip_objects(val)
        else:
            out[key] = val

    return out


def _strip_objects(objects: list) -> list:
    cleaned = []
    for obj in objects:
        clean = {}
        for k, v in obj.items():
            if isinstance(v, bytes):
                clean[k] = f"<binary {len(v)} bytes>"
            elif isinstance(v, list) and k == "_children":
                clean[k] = _strip_objects(v)
            else:
                clean[k] = v

        cleaned.append(clean)

    return cleaned


def cmd_parse(args: argparse.Namespace) -> None:
    vsf = Path(args.file)
    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)

    if not vsf.exists():
        print(f"error: {vsf} not found", file=sys.stderr)
        sys.exit(1)

    result = reader.parse(vsf, extract_bitmaps=not args.json_only, extract_objects=True)
    clean  = _strip_bitmaps(result)

    json_path = out / "theme.json"
    json_path.write_text(json.dumps(clean, indent=2))
    print(f"{json_path}")

    if args.json_only:
        return

    atlas = None
    for bmp in result["bitmaps"]:
        if "image" in bmp:
            atlas = bmp["image"]
            break

    assets = out / "assets"

    slicer.slice_all(clean, assets, atlas)
    print(f"{len(list(assets.glob('*.png')))} assets -> {assets}")


def cmd_parse_all(args: argparse.Namespace) -> None:
    style_dir = Path(args.directory)
    out       = Path(args.output)

    vsf_files = sorted(style_dir.glob("*.vsf"))
    if not vsf_files:
        print(f"no .vsf files in {style_dir}", file=sys.stderr)
        sys.exit(1)

    for vsf in vsf_files:
        ns = argparse.Namespace(file=str(vsf), output=str(out / vsf.stem), json_only=args.json_only)
        try:
            cmd_parse(ns)
        except Exception as e:
            print(f"{vsf.name}: {e}", file=sys.stderr)


def cmd_slice(args: argparse.Namespace) -> None:
    theme_dir = Path(args.directory)
    assets    = theme_dir / "assets"

    data = theme.load(str(theme_dir))

    from .utils import png
    atlas_path = assets / "style.png"
    atlas = png.Bitmap.load_png(atlas_path) if atlas_path.exists() else None

    slicer.slice_all(data, assets, atlas)
    print(f"{len(list(assets.glob('*.png')))} assets -> {assets}")


def cmd_slice_all(args: argparse.Namespace) -> None:
    dirs = theme.theme_dirs(args.directory)
    if not dirs:
        print(f"no theme dirs in {args.directory}", file=sys.stderr)
        sys.exit(1)

    for d in dirs:
        ns = argparse.Namespace(directory=str(d))
        try:
            cmd_slice(ns)
        except Exception as e:
            print(f"{d.name}: {e}", file=sys.stderr)


def cmd_color(args: argparse.Namespace) -> None:
    dirs = theme.theme_dirs(args.directory)

    if args.find:
        target = args.find.lower().lstrip("#")
        for d in dirs:
            data = theme.load(str(d))
            matches = []
            for section in ["colors", "sys_colors"]:
                for k, v in data.get(section, {}).items():
                    if isinstance(v, str) and v.lower().lstrip("#") == target:
                        matches.append(k)

            if matches:
                print(f"{d.name}: {', '.join(matches)}")
    else:
        keys = [k.strip() for k in args.keys.split(",")]
        for d in dirs:
            data  = theme.load(str(d))
            parts = [f"{k}={theme.get_color(data, k) or '?'}" for k in keys]

            print(f"{d.name}: {', '.join(parts)}")


def cmd_object(args: argparse.Namespace) -> None:
    data = theme.load(args.directory)
    obj  = theme.find_object(data["objects"], args.path)

    if not obj:
        print(f"not found: {args.path}", file=sys.stderr)
        sys.exit(1)

    for k, v in obj.items():
        if k == "_children":
            names = [c.get("_name", "?") for c in v]
            print(f"_children: [{', '.join(names)}]")
        else:
            print(f"{k}: {v}")


def cmd_pixel(args: argparse.Namespace) -> None:
    from .utils import png

    img = png.Bitmap.load_png(Path(args.file))
    x, y = args.x, args.y

    if x < 0 or x >= img.width or y < 0 or y >= img.height:
        print(f"({x},{y}) out of bounds ({img.width}x{img.height})", file=sys.stderr)
        sys.exit(1)

    r, g, b, a = img.pixel(x, y)
    print(f"#{r:02x}{g:02x}{b:02x} a={a:02x}")

    if args.row:
        for cx in range(img.width):
            r, g, b, a = img.pixel(cx, y)
            print(f"  {cx}: #{r:02x}{g:02x}{b:02x}({a:02x})")

    if args.col:
        for cy in range(img.height):
            r, g, b, a = img.pixel(x, cy)
            print(f"  {cy}: #{r:02x}{g:02x}{b:02x}({a:02x})")


def main() -> None:
    p   = argparse.ArgumentParser(prog="vsf_parser")
    sub = p.add_subparsers(dest="command", required=True)

    sp = sub.add_parser("parse")
    sp.add_argument("file")
    sp.add_argument("-o", "--output", required=True)
    sp.add_argument("--json-only", action="store_true")
    sp.set_defaults(func=cmd_parse)

    sa = sub.add_parser("parse-all")
    sa.add_argument("directory")
    sa.add_argument("-o", "--output", required=True)
    sa.add_argument("--json-only", action="store_true")
    sa.set_defaults(func=cmd_parse_all)

    ss = sub.add_parser("slice")
    ss.add_argument("directory")
    ss.set_defaults(func=cmd_slice)

    ssa = sub.add_parser("slice-all")
    ssa.add_argument("directory")
    ssa.set_defaults(func=cmd_slice_all)

    sc = sub.add_parser("color")
    sc.add_argument("directory")
    sc.add_argument("-k", "--keys", default="clBtnFace,clWindow,ktcEdit")
    sc.add_argument("-f", "--find")
    sc.set_defaults(func=cmd_color)

    so = sub.add_parser("object")
    so.add_argument("directory")
    so.add_argument("path")
    so.set_defaults(func=cmd_object)

    sx = sub.add_parser("pixel")
    sx.add_argument("file")
    sx.add_argument("x", type=int)
    sx.add_argument("y", type=int)
    sx.add_argument("--row", action="store_true")
    sx.add_argument("--col", action="store_true")
    sx.set_defaults(func=cmd_pixel)

    args = p.parse_args()
    args.func(args)
