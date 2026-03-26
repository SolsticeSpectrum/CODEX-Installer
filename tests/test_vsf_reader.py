import struct
import pytest
from vsf_parser.vsf_reader import read_delphi_string, parse_vsf_header


def _make_delphi_string(text: str) -> bytes:
    """Build a length-prefixed UTF-16LE string the way Delphi writes it."""
    encoded = text.encode("utf-16-le")
    return struct.pack("<I", len(text)) + encoded


def test_read_delphi_string_basic():
    data = _make_delphi_string("Metro Black")
    result, pos = read_delphi_string(data, 0)
    assert result == "Metro Black"
    assert pos == 4 + 11 * 2


def test_read_delphi_string_empty():
    data = struct.pack("<I", 0)
    result, pos = read_delphi_string(data, 0)
    assert result == ""
    assert pos == 4


def test_read_delphi_string_with_offset():
    padding = b"\x00" * 10
    data = padding + _make_delphi_string("Test")
    result, pos = read_delphi_string(data, 10)
    assert result == "Test"
    assert pos == 10 + 4 + 4 * 2


def test_parse_vsf_header_valid():
    data = b"VCL_STYLE 1.0" + b"\x78\x9c" + b"\x00" * 100
    compressed_data = parse_vsf_header(data)
    assert compressed_data is not None


def test_parse_vsf_header_invalid():
    data = b"NOT_A_VSF_FILE"
    with pytest.raises(ValueError, match="Not a VCL_STYLE 1.0 file"):
        parse_vsf_header(data)
