"""CLI interface for the VSF parser."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from .vsf_reader import parse_vsf


def _serialize_result(result: dict) -> dict:
    """Make parse result JSON-serializable by removing PIL images."""
    clean = {}
    for key, value in result.items():
        if key == "bitmaps":
            clean[key] = [
                {k: v for k, v in bmp.items() if k != "image"}
                for bmp in value
            ]
        elif key == "objects":
            clean[key] = _clean_objects(value)
        else:
            clean[key] = value
    return clean


def _clean_objects(objects: list) -> list:
    """Remove non-serializable data from style objects."""
    cleaned = []
    for obj in objects:
        clean_obj = {}
        for k, v in obj.items():
            if isinstance(v, bytes):
                clean_obj[k] = f"<binary {len(v)} bytes>"
            elif isinstance(v, list) and k == "_children":
                clean_obj[k] = _clean_objects(v)
            else:
                clean_obj[k] = v
        cleaned.append(clean_obj)
    return cleaned


def cmd_parse(args: argparse.Namespace) -> None:
    """Parse a single VSF file."""
    vsf_path = Path(args.file)
    if not vsf_path.exists():
        print(f"Error: {vsf_path} not found", file=sys.stderr)
        sys.exit(1)

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    result = parse_vsf(
        vsf_path,
        extract_bitmaps=not args.json_only,
        extract_objects=True,
    )

    # Save JSON
    json_path = output_dir / "theme.json"
    serializable = _serialize_result(result)
    json_path.write_text(json.dumps(serializable, indent=2))
    print(f"Wrote {json_path}")

    # Save bitmaps as PNGs
    if not args.json_only:
        assets_dir = output_dir / "assets"
        assets_dir.mkdir(exist_ok=True)
        for bmp in result["bitmaps"]:
            img = bmp.get("image")
            if img:
                png_name = bmp["name"]
                if not png_name.endswith(".png"):
                    png_name += ".png"
                png_path = assets_dir / png_name
                img.save(png_path)
                print(f"Wrote {png_path} ({img.width}x{img.height})")

        # Generate QSS
        from .qss_generator import generate_qss

        qss_path = output_dir / "style.qss"
        qss_content = generate_qss(serializable, assets_dir)
        qss_path.write_text(qss_content)
        print(f"Wrote {qss_path}")


def cmd_parse_all(args: argparse.Namespace) -> None:
    """Parse all VSF files in a directory."""
    style_dir = Path(args.directory)
    output_dir = Path(args.output)

    vsf_files = sorted(style_dir.glob("*.vsf"))
    if not vsf_files:
        print(f"No .vsf files found in {style_dir}", file=sys.stderr)
        sys.exit(1)

    for vsf_path in vsf_files:
        theme_name = vsf_path.stem
        theme_dir = output_dir / theme_name
        print(f"\n=== Parsing {vsf_path.name} -> {theme_dir} ===")
        ns = argparse.Namespace(
            file=str(vsf_path),
            output=str(theme_dir),
            json_only=args.json_only,
        )
        try:
            cmd_parse(ns)
        except Exception as e:
            print(f"Error parsing {vsf_path.name}: {e}", file=sys.stderr)


def main() -> None:
    parser = argparse.ArgumentParser(
        prog="vsf_parser",
        description="Parse Delphi VCL Style (.vsf) files",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    p_parse = subparsers.add_parser("parse", help="Parse a single VSF file")
    p_parse.add_argument("file", help="Path to .vsf file")
    p_parse.add_argument("-o", "--output", required=True, help="Output directory")
    p_parse.add_argument("--json-only", action="store_true", help="Skip QSS/PNG generation")
    p_parse.set_defaults(func=cmd_parse)

    p_all = subparsers.add_parser("parse-all", help="Parse all VSF files in a directory")
    p_all.add_argument("directory", help="Directory containing .vsf files")
    p_all.add_argument("-o", "--output", required=True, help="Output directory")
    p_all.add_argument("--json-only", action="store_true", help="Skip QSS/PNG generation")
    p_all.set_defaults(func=cmd_parse_all)

    args = parser.parse_args()
    args.func(args)
