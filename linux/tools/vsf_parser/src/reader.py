from __future__ import annotations
import struct
import zlib
from pathlib import Path
from typing import Any

from .constants import VSF_HEADER
from .parsers.stream import read_string
from .parsers.colors import read_colors, read_sys_colors, read_fonts
from .parsers.objects import expand
from .parsers.dfm import parse as parse_dfm
from .parsers import bitmaps


def parse(
    path: Path | str,
    extract_bitmaps: bool = False,
    extract_objects:  bool = False,
) -> dict[str, Any]:

    raw  = Path(path).read_bytes()
    data = _decompress(raw)
    pos  = 0

    # header strings
    name,    pos = read_string(data, pos)
    version, pos = read_string(data, pos)
    author,  pos = read_string(data, pos)
    email,   pos = read_string(data, pos)
    url,     pos = read_string(data, pos)

    # display names block
    dns_size = struct.unpack_from("<q", data, pos)[0]
    pos += 8
    if dns_size > 0:
        pos += dns_size

    # bitmaps
    bmp_count = struct.unpack_from("<i", data, pos)[0]
    pos += 4

    bitmap_list: list[dict[str, Any]] = []
    for i in range(bmp_count):
        if extract_bitmaps:
            bmp, pos = bitmaps.extract(data, pos)
        else:
            bmp, pos = bitmaps.skip(data, pos)

        bmp["index"] = i
        bitmap_list.append(bmp)

    # style objects
    obj_count = struct.unpack_from("<i", data, pos)[0]
    pos += 4

    objects: list[dict[str, Any]] = []
    for _ in range(obj_count):
        class_name, pos = read_string(data, pos)

        size = struct.unpack_from("<I", data, pos)[0]
        pos += 4

        if extract_objects:
            try:
                obj = parse_dfm(data[pos : pos + size])
                obj["_style_class"] = class_name
                expand(obj)
                objects.append(obj)
            except Exception as e:
                objects.append({
                    "_style_class": class_name,
                    "_parse_error": str(e),
                    "_raw_size":    size,
                })

        pos += size

    # colors, syscolors, fonts
    colors,     pos = read_colors(data, pos)
    sys_colors, pos = read_sys_colors(data, pos)
    fonts,      pos = read_fonts(data, pos)

    return {
        "name":         name,
        "version":      version,
        "author":       author,
        "author_email": email,
        "author_url":   url,
        "bitmaps":      bitmap_list,
        "objects":       objects if extract_objects else [],
        "colors":       colors,
        "sys_colors":   sys_colors,
        "fonts":        fonts,
    }


def _decompress(raw: bytes) -> bytes:
    header_len = len(VSF_HEADER)

    if raw[:header_len] != VSF_HEADER:
        raise ValueError(f"not a VCL_STYLE 1.0 file (got {raw[:header_len]!r})")

    return zlib.decompress(raw[header_len:])
