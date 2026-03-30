from __future__ import annotations
import struct

from . import stream
from ..utils import convert
from .. import constants


# 1-byte enum count, then (name, ":", value) triplets
def read_colors(data: bytes, pos: int) -> tuple[dict[str, str], int]:
    count = data[pos] + 1
    pos  += 1

    colors: dict[str, str] = {}

    for i in range(count):
        name, pos  = stream.read_string(data, pos)
        _,    pos  = stream.read_string(data, pos)  # ":"
        val,  pos  = stream.read_string(data, pos)

        if i < len(constants.STYLE_COLORS):
            colors[constants.STYLE_COLORS[i]] = convert.bgr_to_rgb(val)

    return colors, pos


# int32 count, then (name, ":", value) triplets
def read_sys_colors(data: bytes, pos: int) -> tuple[dict[str, str], int]:
    count = struct.unpack_from("<i", data, pos)[0]
    pos  += 4

    colors: dict[str, str] = {}

    for i in range(count):
        name, pos = stream.read_string(data, pos)
        _,    pos = stream.read_string(data, pos)
        val,  pos = stream.read_string(data, pos)

        if i < len(constants.SYS_COLORS):
            colors[constants.SYS_COLORS[i]] = convert.bgr_to_rgb(val)

    return colors, pos


# 1-byte enum count, then (name, ":", font_spec) triplets
def read_fonts(data: bytes, pos: int) -> tuple[dict[str, dict], int]:
    count = data[pos] + 1
    pos  += 1

    fonts: dict[str, dict] = {}

    for i in range(count):
        name, pos = stream.read_string(data, pos)
        _,    pos = stream.read_string(data, pos)
        val,  pos = stream.read_string(data, pos)

        if i < len(constants.STYLE_FONTS):
            fonts[constants.STYLE_FONTS[i]] = convert.parse_font(val)

    return fonts, pos
