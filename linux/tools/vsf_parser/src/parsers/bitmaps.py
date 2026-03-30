from __future__ import annotations
import struct
from typing import Any

from . import stream
from ..utils import png


# name + int32 w + int32 h + w*h*4 BGRA pixels + 2 bool flags
def extract(data: bytes, pos: int) -> tuple[dict[str, Any], int]:
    name, pos = stream.read_string(data, pos)

    w = struct.unpack_from("<i", data, pos)[0]
    pos += 4

    h = struct.unpack_from("<i", data, pos)[0]
    pos += 4

    info: dict[str, Any] = {"name": name, "width": w, "height": h}
    pixels = w * h * 4

    if h > 0 and w > 0:
        raw   = data[pos : pos + pixels]
        pos  += pixels
        info["image"] = png.Bitmap.from_bgra(w, h, raw, flip_y=True)
    else:
        pos += pixels

    info["transparent"] = struct.unpack_from("<?", data, pos)[0]
    pos += 1
    
    info["alpha"]       = struct.unpack_from("<?", data, pos)[0]
    pos += 1

    return info, pos


def skip(data: bytes, pos: int) -> tuple[dict[str, Any], int]:
    name, pos = stream.read_string(data, pos)

    w = struct.unpack_from("<i", data, pos)[0]
    pos += 4
    
    h = struct.unpack_from("<i", data, pos)[0]
    pos += 4
    
    pos += w * h * 4 + 2

    return {"name": name, "width": w, "height": h}, pos
