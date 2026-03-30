from __future__ import annotations
import struct
import zlib
from pathlib import Path
from typing import Any

from . import constants
from .parsers import stream, dfm, colors, objects, bitmaps


def parse(
    path: Path | str,
    extract_bitmaps: bool = False,
    extract_objects:  bool = False,
) -> dict[str, Any]:

    raw  = Path(path).read_bytes()
    data = _decompress(raw)
    pos  = 0

    # header strings
    name,    pos = stream.read_string(data, pos)
    version, pos = stream.read_string(data, pos)
    author,  pos = stream.read_string(data, pos)
    email,   pos = stream.read_string(data, pos)
    url,     pos = stream.read_string(data, pos)

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

    object_list: list[dict[str, Any]] = []
    for _ in range(obj_count):
        class_name, pos = stream.read_string(data, pos)

        size = struct.unpack_from("<I", data, pos)[0]
        pos += 4

        if extract_objects:
            try:
                obj = dfm.parse(data[pos : pos + size])
                obj["_style_class"] = class_name
                objects.expand(obj)
                object_list.append(obj)
            except Exception as e:
                object_list.append({
                    "_style_class": class_name,
                    "_parse_error": str(e),
                    "_raw_size":    size,
                })

        pos += size

    color_map,     pos = colors.read_colors(data, pos)
    sys_color_map, pos = colors.read_sys_colors(data, pos)
    font_map,      pos = colors.read_fonts(data, pos)

    return {
        "name":         name,
        "version":      version,
        "author":       author,
        "author_email": email,
        "author_url":   url,
        "bitmaps":      bitmap_list,
        "objects":      object_list if extract_objects else [],
        "colors":       color_map,
        "sys_colors":   sys_color_map,
        "fonts":        font_map,
    }


def _decompress(raw: bytes) -> bytes:
    header_len = len(constants.VSF_HEADER)

    if raw[:header_len] != constants.VSF_HEADER:
        raise ValueError(f"not a VCL_STYLE 1.0 file (got {raw[:header_len]!r})")

    return zlib.decompress(raw[header_len:])
