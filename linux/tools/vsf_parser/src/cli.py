from __future__ import annotations
import argparse
import json
import sys
from pathlib import Path

from .reader import parse
from .slicer import slice_all


def _clean(result: dict) -> dict:
    out = {}

    for key, val in result.items():
        if key == "bitmaps":
            out[key] = [{k: v for k, v in bmp.items() if k != "image"} for bmp in val]
        elif key == "objects":
            out[key] = _clean_objects(val)
        else:
            out[key] = val

    return out


def _clean_objects(objects: list) -> list:
    cleaned = []

    for obj in objects:
        clean = {}
        for k, v in obj.items():
            if isinstance(v, bytes):
                clean[k] = f"<binary {len(v)} bytes>"
            elif isinstance(v, list) and k == "_children":
                clean[k] = _clean_objects(v)
            else:
                clean[k] = v
        cleaned.append(clean)

    return cleaned


def cmd_parse(args: argparse.Namespace) -> None:
    vsf  = Path(args.file)
    out  = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)

    if not vsf.exists():
        print(f"error: {vsf} not found", file=sys.stderr)
        sys.exit(1)

    result = parse(vsf, extract_bitmaps=not args.json_only, extract_objects=True)
    clean  = _clean(result)

    # json
    json_path = out / "theme.json"
    json_path.write_text(json.dumps(clean, indent=2))
    print(f"wrote {json_path}")

    if args.json_only:
        return

    # slice atlas
    atlas = None
    for bmp in result["bitmaps"]:
        if "image" in bmp:
            atlas = bmp["image"]
            break

    assets = out / "assets"
    slice_all(clean, assets, atlas)
    print(f"wrote {len(list(assets.glob('*.png')))} PNGs to {assets}")


def cmd_parse_all(args: argparse.Namespace) -> None:
    style_dir = Path(args.directory)
    out       = Path(args.output)

    vsf_files = sorted(style_dir.glob("*.vsf"))
    if not vsf_files:
        print(f"no .vsf files in {style_dir}", file=sys.stderr)
        sys.exit(1)

    for vsf in vsf_files:
        theme_dir = out / vsf.stem
        print(f"\n=== {vsf.name} -> {theme_dir} ===")

        ns = argparse.Namespace(file=str(vsf), output=str(theme_dir), json_only=args.json_only)
        try:
            cmd_parse(ns)
        except Exception as e:
            print(f"error: {e}", file=sys.stderr)


def main() -> None:
    p = argparse.ArgumentParser(prog="vsf_parser")
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

    args = p.parse_args()
    args.func(args)
