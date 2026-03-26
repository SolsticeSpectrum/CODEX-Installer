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


# --- Real file tests ---

from pathlib import Path
from vsf_parser.vsf_reader import parse_vsf

STYLE_DIR = Path(__file__).parent.parent / "installer" / "src" / "Include" / "Style"


@pytest.mark.skipif(
    not (STYLE_DIR / "CODEX.vsf").exists(),
    reason="CODEX.vsf not found",
)
def test_parse_vsf_codex_metadata():
    result = parse_vsf(STYLE_DIR / "CODEX.vsf")
    assert result["name"] != ""
    assert isinstance(result["version"], str)
    assert isinstance(result["author"], str)


@pytest.mark.skipif(
    not (STYLE_DIR / "CODEX.vsf").exists(),
    reason="CODEX.vsf not found",
)
def test_parse_vsf_codex_colors():
    result = parse_vsf(STYLE_DIR / "CODEX.vsf")
    colors = result["colors"]
    assert "ktcBorder" in colors
    assert "ktcButton" in colors
    assert "ktcWindow" in colors
    for name, value in colors.items():
        assert value.startswith("#") or value.startswith("cl"), \
            f"Color {name} has unexpected format: {value}"


@pytest.mark.skipif(
    not (STYLE_DIR / "CODEX.vsf").exists(),
    reason="CODEX.vsf not found",
)
def test_parse_vsf_codex_fonts():
    result = parse_vsf(STYLE_DIR / "CODEX.vsf")
    fonts = result["fonts"]
    assert "ktfButtonTextNormal" in fonts
    btn_font = fonts["ktfButtonTextNormal"]
    assert "name" in btn_font
    assert "size" in btn_font
    assert "color" in btn_font


@pytest.mark.skipif(
    not (STYLE_DIR / "CODEX.vsf").exists(),
    reason="CODEX.vsf not found",
)
def test_parse_vsf_codex_sys_colors():
    result = parse_vsf(STYLE_DIR / "CODEX.vsf")
    sys_colors = result["sys_colors"]
    assert "clWindowText" in sys_colors
    assert "clBtnFace" in sys_colors


@pytest.mark.skipif(
    not (STYLE_DIR / "CODEX.vsf").exists(),
    reason="CODEX.vsf not found",
)
def test_parse_vsf_codex_bitmaps():
    from PIL import Image

    result = parse_vsf(STYLE_DIR / "CODEX.vsf", extract_bitmaps=True)
    bitmaps = result["bitmaps"]
    assert len(bitmaps) >= 1
    first = bitmaps[0]
    assert "image" in first
    img = first["image"]
    assert isinstance(img, Image.Image)
    assert img.width > 0
    assert img.height > 0


@pytest.mark.skipif(
    not (STYLE_DIR / "CODEX.vsf").exists(),
    reason="CODEX.vsf not found",
)
def test_parse_vsf_codex_objects():
    result = parse_vsf(STYLE_DIR / "CODEX.vsf", extract_objects=True)
    objects = result["objects"]
    assert len(objects) > 0
    # Check that objects have class info
    names = [o.get("_name", "") for o in objects]
    assert any(names), f"No named objects found"
    # Check no parse errors
    errors = [o for o in objects if "_parse_error" in o]
    assert len(errors) == 0, f"Parse errors: {errors}"
