from __future__ import annotations
import struct
from typing import Any

from . import stream, dfm
from .. import constants


def parse_children(blob: bytes) -> list[dict[str, Any]]:
    pos = 0

    raw = struct.unpack_from("<i", blob, pos)[0]
    pos += 4

    # count has $F0000 flag OR'd in for new format
    if raw & constants.OBJECT_FLAG == constants.OBJECT_FLAG:
        count = raw & ~constants.OBJECT_FLAG
    else:
        count = raw

    children = []

    for _ in range(count):
        if pos >= len(blob):
            break

        class_name, pos = stream.read_string(blob, pos)

        if pos + 4 > len(blob):
            break

        size = struct.unpack_from("<I", blob, pos)[0]
        pos += 4

        if pos + size > len(blob):
            break

        dfm_data = blob[pos : pos + size]
        pos += size

        try:
            obj = dfm.parse(dfm_data)
            obj["_style_class"] = class_name
            expand(obj)
            children.append(obj)
        except Exception as e:
            children.append({
                "_style_class": class_name,
                "_parse_error": str(e),
                "_raw_size":    size,
            })

    return children


def expand(obj: dict[str, Any]) -> None:
    # if "Objects" is a binary blob, parse it as nested style children
    if "Objects" in obj and isinstance(obj["Objects"], bytes):
        blob = obj["Objects"]
        if len(blob) >= 4:
            try:
                obj["_children"] = parse_children(blob)
                del obj["Objects"]
            except Exception:
                pass

    if "_children" in obj:
        for child in obj["_children"]:
            expand(child)
