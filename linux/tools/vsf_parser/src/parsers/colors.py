from __future__ import annotations
import struct

from .stream import read_string
from ..utils.convert import bgr_to_rgb, parse_font
from ..constants import STYLE_COLORS, SYS_COLORS, STYLE_FONTS


# 1-byte enum count, then (name, ":", value) triplets
def read_colors(data: bytes, pos: int) -> tuple[dict[str, str], int]:
    count = data[pos] + 1
    pos  += 1

    colors: dict[str, str] = {}

    for i in range(count):
        name, pos  = read_string(data, pos)
        _,    pos  = read_string(data, pos)  # ":"
        val,  pos  = read_string(data, pos)

        if i < len(STYLE_COLORS):
            colors[STYLE_COLORS[i]] = bgr_to_rgb(val)

    return colors, pos


# int32 count, then (name, ":", value) triplets
def read_sys_colors(data: bytes, pos: int) -> tuple[dict[str, str], int]:
    count = struct.unpack_from("<i", data, pos)[0]
    pos  += 4

    colors: dict[str, str] = {}

    for i in range(count):
        name, pos = read_string(data, pos)
        _,    pos = read_string(data, pos)
        val,  pos = read_string(data, pos)

        if i < len(SYS_COLORS):
            colors[SYS_COLORS[i]] = bgr_to_rgb(val)

    return colors, pos


# 1-byte enum count, then (name, ":", font_spec) triplets
def read_fonts(data: bytes, pos: int) -> tuple[dict[str, dict], int]:
    count = data[pos] + 1
    pos  += 1

    fonts: dict[str, dict] = {}

    for i in range(count):
        name, pos = read_string(data, pos)
        _,    pos = read_string(data, pos)
        val,  pos = read_string(data, pos)

        if i < len(STYLE_FONTS):
            fonts[STYLE_FONTS[i]] = parse_font(val)

    return fonts, pos
