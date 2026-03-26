"""Delphi DFM binary (TPF0) format parser.

Parses the binary component stream format used by Delphi's
TStream.WriteComponent/ReadComponent methods.

TPF0 format:
  - 4 bytes: "TPF0" signature
  - Class name: length-prefixed string (1-byte length)
  - Instance name: length-prefixed string (1-byte length)
  - Properties: sequence of (name, type, value) until name is empty
  - Children: nested objects until end marker (0x00)

Property types:
  0x00 = end of properties / end of object
  0x01 = list begin
  0x02 = int8
  0x03 = int16
  0x04 = int32
  0x05 = extended float (10 bytes in Delphi)
  0x06 = short string (1-byte length prefix)
  0x07 = ident/enum (1-byte length prefix string)
  0x08 = false
  0x09 = true
  0x0A = binary data (4-byte length prefix)
  0x0B = set (sequence of ident strings until empty string)
  0x0C = long string (4-byte length prefix)
  0x0D = nil/null
  0x12 = UTF-8 string (4-byte length prefix)
  0x14 = int64
"""

from __future__ import annotations

import struct
from typing import Any


def _read_short_string(data: bytes, pos: int) -> tuple[str, int]:
    """Read a 1-byte length-prefixed string."""
    length = data[pos]
    pos += 1
    value = data[pos : pos + length].decode("ascii", errors="replace")
    pos += length
    return value, pos


def _read_property_value(data: bytes, pos: int, type_byte: int) -> tuple[Any, int]:
    """Read a property value based on its type byte."""
    if type_byte == 0x01:
        # List - read items until 0x00 end marker
        items = []
        while pos < len(data):
            item_type = data[pos]
            if item_type == 0x00:
                pos += 1
                break
            pos += 1
            value, pos = _read_property_value(data, pos, item_type)
            items.append(value)
        return items, pos

    if type_byte == 0x02:
        value = struct.unpack_from("<b", data, pos)[0]
        return value, pos + 1

    if type_byte == 0x03:
        value = struct.unpack_from("<h", data, pos)[0]
        return value, pos + 2

    if type_byte == 0x04:
        value = struct.unpack_from("<i", data, pos)[0]
        return value, pos + 4

    if type_byte == 0x05:
        # Delphi extended is 10 bytes; extract as raw bytes
        raw = data[pos : pos + 10]
        # Try to interpret as 80-bit extended float
        # Fallback: just store as hex string
        try:
            # Simple approach: use the first 8 bytes as double approximation
            value = struct.unpack_from("<d", data, pos)[0]
        except struct.error:
            value = 0.0
        return value, pos + 10

    if type_byte == 0x06:
        return _read_short_string(data, pos)

    if type_byte == 0x07:
        return _read_short_string(data, pos)

    if type_byte == 0x08:
        return False, pos

    if type_byte == 0x09:
        return True, pos

    if type_byte == 0x0A:
        length = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        value = data[pos : pos + length]
        return value, pos + length

    if type_byte == 0x0B:
        # Set - sequence of ident strings until empty
        items = []
        while pos < len(data):
            s, pos = _read_short_string(data, pos)
            if s == "":
                break
            items.append(s)
        return items, pos

    if type_byte == 0x0C:
        length = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        value = data[pos : pos + length].decode("ascii", errors="replace")
        return value, pos + length

    if type_byte == 0x0D:
        return None, pos

    if type_byte == 0x12:
        length = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        value = data[pos : pos + length].decode("utf-8", errors="replace")
        return value, pos + length

    if type_byte == 0x14:
        value = struct.unpack_from("<q", data, pos)[0]
        return value, pos + 8

    raise ValueError(f"Unknown DFM property type: 0x{type_byte:02x} at pos {pos}")


def _parse_object(data: bytes, pos: int) -> tuple[dict[str, Any], int]:
    """Parse a single DFM object (class + name + properties + children)."""
    class_name, pos = _read_short_string(data, pos)
    instance_name, pos = _read_short_string(data, pos)

    obj: dict[str, Any] = {
        "_class": class_name,
        "_name": instance_name,
    }

    # Read properties until empty name (0x00 length byte)
    while pos < len(data):
        if data[pos] == 0x00:
            pos += 1  # consume end-of-properties marker
            break
        prop_name, pos = _read_short_string(data, pos)
        type_byte = data[pos]
        pos += 1
        value, pos = _read_property_value(data, pos, type_byte)
        obj[prop_name] = value

    # Read child objects until end marker (0x00)
    children = []
    while pos < len(data) and data[pos] != 0x00:
        child, pos = _parse_object(data, pos)
        children.append(child)

    if pos < len(data) and data[pos] == 0x00:
        pos += 1  # consume end-of-children marker

    if children:
        obj["_children"] = children

    return obj, pos


def parse_dfm_binary(data: bytes) -> dict[str, Any]:
    """Parse a TPF0 binary DFM stream into a dict.

    The data should start with the 'TPF0' signature.
    Returns a nested dict with '_class', '_name', property values,
    and optional '_children' list.
    """
    if data[:4] != b"TPF0":
        raise ValueError(f"Not a TPF0 stream (got {data[:4]!r})")
    pos = 4
    obj, _ = _parse_object(data, pos)
    return obj
