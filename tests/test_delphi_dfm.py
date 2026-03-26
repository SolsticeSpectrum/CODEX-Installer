import struct
import pytest
from vsf_parser.delphi_dfm import parse_dfm_binary


def test_parse_dfm_empty_object():
    """TPF0 + class name + instance name + no properties + end marker."""
    data = b"TPF0"
    data += b"\x0BTSeStyleObj"  # class name
    data += b"\x04Test"  # instance name
    data += b"\x00"  # end of properties
    data += b"\x00"  # end of children
    result = parse_dfm_binary(data)
    assert result["_class"] == "TSeStyleObj"
    assert result["_name"] == "Test"


def test_parse_dfm_integer_property():
    data = b"TPF0"
    data += b"\x0BTSeStyleObj"
    data += b"\x04Test"
    # Property: "Left" = 10 (int16, type 0x03)
    data += b"\x04Left"
    data += b"\x03"
    data += struct.pack("<h", 10)
    data += b"\x00"  # end props
    data += b"\x00"  # end children
    result = parse_dfm_binary(data)
    assert result["Left"] == 10


def test_parse_dfm_string_property():
    data = b"TPF0"
    data += b"\x0BTSeStyleObj"
    data += b"\x04Test"
    # Property: "Name" = "style.png" (string type 0x06)
    data += b"\x04Name"
    data += b"\x06"
    data += b"\x09style.png"
    data += b"\x00"  # end props
    data += b"\x00"  # end children
    result = parse_dfm_binary(data)
    assert result["Name"] == "style.png"


def test_parse_dfm_bool_properties():
    data = b"TPF0"
    data += b"\x0BTSeStyleObj"
    data += b"\x04Test"
    data += b"\x07Visible"
    data += b"\x09"  # True
    data += b"\x06Masked"
    data += b"\x08"  # False
    data += b"\x00"
    data += b"\x00"
    result = parse_dfm_binary(data)
    assert result["Visible"] is True
    assert result["Masked"] is False


def test_parse_dfm_enum_property():
    data = b"TPF0"
    data += b"\x0BTSeStyleObj"
    data += b"\x04Test"
    data += b"\x09TileStyle"
    data += b"\x07"  # ident type
    data += b"\x08tsCenter"
    data += b"\x00"
    data += b"\x00"
    result = parse_dfm_binary(data)
    assert result["TileStyle"] == "tsCenter"


def test_parse_dfm_invalid_signature():
    with pytest.raises(ValueError, match="Not a TPF0 stream"):
        parse_dfm_binary(b"XXXX")
