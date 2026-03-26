"""VSF 1.0 binary format parser.

Parses Delphi VCL Style files based on the format defined in
Embarcadero's StyleAPI.inc and StyleUtils.inc.
"""

from __future__ import annotations

import struct
import zlib
from pathlib import Path
from typing import Any


VSF_HEADER = b"VCL_STYLE 1.0"
VSF_HEADER_LEN = 13


def read_delphi_string(data: bytes, pos: int) -> tuple[str, int]:
    """Read a Delphi length-prefixed UTF-16LE string.

    Format: int32 char_count, then char_count * 2 bytes of UTF-16LE.
    Returns (string_value, new_position).
    """
    char_count = struct.unpack_from("<I", data, pos)[0]
    pos += 4
    byte_count = char_count * 2
    text = data[pos : pos + byte_count].decode("utf-16-le")
    pos += byte_count
    return text, pos


def parse_vsf_header(data: bytes) -> bytes:
    """Validate VSF header and return the compressed payload.

    Raises ValueError if header doesn't match VCL_STYLE 1.0.
    """
    if data[:VSF_HEADER_LEN] != VSF_HEADER:
        raise ValueError(
            f"Not a VCL_STYLE 1.0 file (got {data[:VSF_HEADER_LEN]!r})"
        )
    return data[VSF_HEADER_LEN:]
