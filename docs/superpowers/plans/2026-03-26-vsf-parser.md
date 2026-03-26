# VSF Parser Tool Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Parse Delphi VCL Style (.vsf) files and produce JSON theme data + pre-sliced PNG assets + Qt6 QSS stylesheets for the Linux port of CODEX-Installer.

**Architecture:** Three-layer pipeline: `vsf_reader.py` parses the binary format into Python dicts, `qss_generator.py` converts parsed data to QSS + sliced PNGs, `cli.py` orchestrates with argparse. All output goes to per-theme directories.

**Tech Stack:** Python 3.12+, Pillow (bitmap handling), stdlib only otherwise (zlib, struct, json, argparse, pathlib)

---

## File Structure

```
tools/vsf_parser/
  __init__.py          # package marker
  __main__.py          # entry point for `python -m vsf_parser`
  cli.py               # argument parsing, orchestration
  vsf_reader.py        # VSF 1.0 binary format parser
  qss_generator.py     # parsed data -> QSS + sliced PNGs
  delphi_dfm.py        # Delphi DFM binary (TPF0) deserializer
tests/
  test_vsf_reader.py   # unit tests for the parser
  test_qss_generator.py # unit tests for QSS generation
  test_delphi_dfm.py   # unit tests for DFM parsing
  fixtures/            # small hand-crafted binary test data
```

---

### Task 1: Project scaffolding and venv

**Files:**
- Create: `tools/vsf_parser/__init__.py`
- Create: `tools/vsf_parser/__main__.py`
- Create: `tools/vsf_parser/requirements.txt`
- Create: `tests/__init__.py`

- [ ] **Step 1: Create venv and install dependencies**

```bash
cd /home/user/github/CODEX-Installer
python3 -m venv tools/vsf_parser/.venv
source tools/vsf_parser/.venv/bin/activate
pip install Pillow pytest
pip freeze > tools/vsf_parser/requirements.txt
```

- [ ] **Step 2: Create package files**

Create `tools/vsf_parser/__init__.py`:
```python
"""VSF Parser - Delphi VCL Style file parser for Linux Qt6 port."""
```

Create `tools/vsf_parser/__main__.py`:
```python
"""Entry point for python -m vsf_parser."""
from .cli import main

if __name__ == "__main__":
    main()
```

Create `tests/__init__.py`:
```python
```

- [ ] **Step 3: Verify setup**

```bash
source tools/vsf_parser/.venv/bin/activate
python -c "from PIL import Image; print('Pillow OK')"
python -c "import pytest; print('pytest OK')"
```

Expected: Both print OK.

- [ ] **Step 4: Commit**

```bash
git add tools/vsf_parser/__init__.py tools/vsf_parser/__main__.py tools/vsf_parser/requirements.txt tests/__init__.py
git commit -m "feat: scaffold vsf_parser package with venv"
```

---

### Task 2: Delphi string and header parsing

**Files:**
- Create: `tools/vsf_parser/vsf_reader.py`
- Create: `tests/test_vsf_reader.py`

- [ ] **Step 1: Write failing test for read_delphi_string**

Create `tests/test_vsf_reader.py`:
```python
import struct
import pytest
from vsf_parser.vsf_reader import read_delphi_string


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
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cd /home/user/github/CODEX-Installer
source tools/vsf_parser/.venv/bin/activate
PYTHONPATH=tools python -m pytest tests/test_vsf_reader.py -v
```

Expected: FAIL with `ModuleNotFoundError: No module named 'vsf_parser.vsf_reader'`

- [ ] **Step 3: Implement read_delphi_string**

Create `tools/vsf_parser/vsf_reader.py`:
```python
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
```

- [ ] **Step 4: Run test to verify it passes**

```bash
PYTHONPATH=tools python -m pytest tests/test_vsf_reader.py -v
```

Expected: 3 tests PASS

- [ ] **Step 5: Write failing test for parse_vsf_header**

Add to `tests/test_vsf_reader.py`:
```python
from vsf_parser.vsf_reader import parse_vsf_header


def test_parse_vsf_header_valid():
    data = b"VCL_STYLE 1.0" + b"\x78\x9c" + b"\x00" * 100
    compressed_data = parse_vsf_header(data)
    assert compressed_data is not None


def test_parse_vsf_header_invalid():
    data = b"NOT_A_VSF_FILE"
    with pytest.raises(ValueError, match="Not a VCL_STYLE 1.0 file"):
        parse_vsf_header(data)
```

- [ ] **Step 6: Implement parse_vsf_header**

Add to `tools/vsf_parser/vsf_reader.py`:
```python
def parse_vsf_header(data: bytes) -> bytes:
    """Validate VSF header and return the compressed payload.

    Raises ValueError if header doesn't match VCL_STYLE 1.0.
    """
    if data[:VSF_HEADER_LEN] != VSF_HEADER:
        raise ValueError(
            f"Not a VCL_STYLE 1.0 file (got {data[:VSF_HEADER_LEN]!r})"
        )
    return data[VSF_HEADER_LEN:]
```

- [ ] **Step 7: Run tests**

```bash
PYTHONPATH=tools python -m pytest tests/test_vsf_reader.py -v
```

Expected: 5 tests PASS

- [ ] **Step 8: Commit**

```bash
git add tools/vsf_parser/vsf_reader.py tests/test_vsf_reader.py
git commit -m "feat: vsf_reader with delphi string and header parsing"
```

---

### Task 3: Parse VSF metadata (name, version, author, colors, fonts)

**Files:**
- Modify: `tools/vsf_parser/vsf_reader.py`
- Modify: `tests/test_vsf_reader.py`

- [ ] **Step 1: Write failing test using real CODEX.vsf**

Add to `tests/test_vsf_reader.py`:
```python
from pathlib import Path
from vsf_parser.vsf_reader import parse_vsf


STYLE_DIR = Path(__file__).parent.parent / "installer" / "src" / "Include" / "Style"


@pytest.mark.skipif(
    not (STYLE_DIR / "CODEX.vsf").exists(),
    reason="CODEX.vsf not found",
)
def test_parse_vsf_codex_metadata():
    result = parse_vsf(STYLE_DIR / "CODEX.vsf")
    # The CODEX.vsf is actually "Metro Black" theme renamed
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
    # Colors should be #RRGGBB format
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
```

- [ ] **Step 2: Run tests to verify they fail**

```bash
PYTHONPATH=tools python -m pytest tests/test_vsf_reader.py::test_parse_vsf_codex_metadata -v
```

Expected: FAIL with `ImportError` (parse_vsf not defined)

- [ ] **Step 3: Implement color/font parsing helpers and parse_vsf**

Add to `tools/vsf_parser/vsf_reader.py`:
```python
# Color enum names in order (from StyleAPI.inc TSeStyleColor)
STYLE_COLOR_NAMES = [
    "ktcBorder", "ktcCategoryButtons", "ktcCategoryPanelGroup",
    "ktcComboBox", "ktcComboBoxDisabled", "ktcButton", "ktcButtonHot",
    "ktcButtonPressed", "ktcButtonFocused", "ktcButtonDisabled",
    "ktcEdit", "ktcEditDisabled", "ktcGrid",
    "ktcGenericGradientBase", "ktcGenericGradientEnd",
    "ktcHintGradientBase", "ktcListBox", "ktcListBoxDisabled",
    "ktcListView", "ktcPanel", "ktcPanelDisabled", "ktcTreeView",
    "ktcWindow", "ktcSplitter", "ktcCategoryButtonsGradientBase",
    "ktcCategoryButtonsGradientEnd", "ktcToolBarGradientBase",
    "ktcToolBarGradientEnd", "ktcGenericBackground", "ktcHintGradientEnd",
    "ktcFocusEffect", "ktsAlternatingRowBackground",
]

# System color names (from StyleAPI.inc SysColors array)
SYS_COLOR_NAMES = [
    "clActiveBorder", "clActiveCaption", "clBtnFace", "clBtnHighlight",
    "clBtnShadow", "clBtnText", "clCaptionText", "clGrayText",
    "clHighlight", "clHighlightText", "clInactiveBorder",
    "clInactiveCaption", "clInactiveCaptionText", "clInfoBk",
    "clInfoText", "clMenu", "clMenuText", "clScrollBar",
    "cl3DDkShadow", "cl3DLight", "clWindow", "clWindowFrame",
    "clWindowText",
]

# Font enum names in order (from StyleAPI.inc TSeStyleFont)
STYLE_FONT_NAMES = [
    "ktfCaptionTextNormal", "ktfCaptionTextInactive",
    "ktfSmCaptionTextNormal", "ktfSmCaptionTextInactive",
    "ktfStaticTextNormal", "ktfStaticTextHot",
    "ktfStaticTextFocused", "ktfStaticTextDisabled",
    "ktfPanelTextNormal", "ktfPanelTextDisabled",
    "ktfButtonTextNormal", "ktfButtonTextPressed",
    "ktfButtonTextHot", "ktfButtonTextFocused",
    "ktfButtonTextDisabled", "ktfCheckBoxTextNormal",
    "ktfCheckBoxTextPressed", "ktfCheckBoxTextHot",
    "ktfCheckBoxTextFocused", "ktfCheckBoxTextDisabled",
    "ktfRadioButtonTextNormal", "ktfRadioButtonTextPressed",
    "ktfRadioButtonTextHot", "ktfRadioButtonTextFocused",
    "ktfRadioButtonTextDisabled", "ktfGroupBoxTextNormal",
    "ktfGroupBoxTextDisabled", "ktfWindowTextNormal",
    "ktfWindowTextDisabled", "ktfEditBoxTextNormal",
    "ktfEditBoxTextFocused", "ktfEditBoxTextHot",
    "ktfEditBoxTextDisabled", "ktfEditBoxTextSelected",
    "ktfMenuItemTextNormal", "ktfMenuItemTextSelected",
    "ktfMenuItemTextHot", "ktfMenuItemTextDisabled",
    "ktfToolItemTextNormal", "ktfToolItemTextSelected",
    "ktfToolItemTextHot", "ktfToolItemTextDisabled",
    "ktfHeaderSectionTextNormal", "ktfHeaderSectionTextPressed",
    "ktfHeaderSectionTextDraggedOut", "ktfHeaderSectionTextDragging",
    "ktfHeaderSectionTextHot", "ktfHeaderSectionTextUnderDrag",
    "ktfHeaderSectionTextDisabled", "ktfStatusPanelTextNormal",
    "ktfStatusPanelTextDisabled", "ktfTabTextInactiveNormal",
    "ktfTabTextInactiveHot", "ktfTabTextInactiveDisabled",
    "ktfTabTextActiveNormal", "ktfTabTextActiveHot",
    "ktfTabTextActiveDisabled", "ktfListItemTextNormal",
    "ktfListItemTextHot", "ktfListItemTextSelected",
    "ktfListItemTextFocused", "ktfListItemTextDisabled",
    "ktfPopupMenuItemTextNormal", "ktfPopupMenuItemTextSelected",
    "ktfPopupMenuItemTextHot", "ktfPopupMenuItemTextDisabled",
    "ktfCatgeoryButtonsNormal", "ktfCatgeoryButtonsHot",
    "ktfCatgeoryButtonsSelected", "ktfCatgeoryButtonsCategoryNormal",
    "ktfCatgeoryButtonsCategorySelected",
    "ktfCategoryPanelGroupHeaderNormal",
    "ktfCategoryPanelGroupHeaderHot", "ktfComboBoxItemNormal",
    "ktfComboBoxItemFocused", "ktfComboBoxItemHot",
    "ktfComboBoxItemDisabled", "ktfComboBoxItemSelected",
    "ktfGridItemNormal", "ktfGridItemSelected",
    "ktfGridItemFixedNormal", "ktfGridItemFixedHot",
    "ktfGridItemFixedPressed", "ktfTreeItemTextNormal",
    "ktfTreeItemTextHot", "ktfTreeItemTextSelected",
    "ktfTreeItemTextFocused", "ktfTreeItemTextDisabled",
]

# Well-known Delphi color constants mapped to hex
DELPHI_NAMED_COLORS = {
    "clBlack": "#000000", "clMaroon": "#800000", "clGreen": "#008000",
    "clOlive": "#808000", "clNavy": "#000080", "clPurple": "#800080",
    "clTeal": "#008080", "clGray": "#808080", "clSilver": "#c0c0c0",
    "clRed": "#ff0000", "clLime": "#00ff00", "clYellow": "#ffff00",
    "clBlue": "#0000ff", "clFuchsia": "#ff00ff", "clAqua": "#00ffff",
    "clWhite": "#ffffff", "clNone": "#000000",
}


def convert_delphi_color(color_str: str) -> str:
    """Convert Delphi color string to #RRGGBB.

    Delphi uses $00BBGGRR format (BGR byte order).
    Named colors like clBlack are mapped via lookup.
    """
    color_str = color_str.strip()
    if color_str in DELPHI_NAMED_COLORS:
        return DELPHI_NAMED_COLORS[color_str]
    if color_str.startswith("$00") and len(color_str) == 9:
        bgr = color_str[3:]
        r = bgr[4:6]
        g = bgr[2:4]
        b = bgr[0:2]
        return f"#{r}{g}{b}".lower()
    if color_str.startswith("$") and len(color_str) >= 7:
        # Handle other $ formats by stripping prefix and swapping
        hex_part = color_str[1:].zfill(8)
        bgr = hex_part[-6:]
        r = bgr[4:6]
        g = bgr[2:4]
        b = bgr[0:2]
        return f"#{r}{g}{b}".lower()
    return color_str


def parse_font_string(font_str: str) -> dict[str, Any]:
    """Parse Delphi font spec: 'FontName,Size,Charset,R,G,B[,style]'."""
    parts = font_str.split(",")
    result: dict[str, Any] = {
        "name": parts[0] if len(parts) > 0 else "Tahoma",
        "size": int(parts[1]) if len(parts) > 1 else 8,
        "charset": int(parts[2]) if len(parts) > 2 else 1,
        "color": "#000000",
        "style": "",
    }
    if len(parts) >= 6:
        r, g, b = int(parts[3]), int(parts[4]), int(parts[5])
        result["color"] = f"#{r:02x}{g:02x}{b:02x}"
    if len(parts) >= 7:
        result["style"] = parts[6].strip()
    return result


def _read_color_table(data: bytes, pos: int, expected_names: list[str]) -> tuple[dict[str, str], int]:
    """Read a Colors or SysColors section from the decompressed stream.

    Format: int32 count, then count * (name_string, ':', color_string) triplets.
    """
    count = struct.unpack_from("<i", data, pos)[0]
    pos += 4
    # For enum-typed counts (TSeStyleColor), interpret as enum ordinal
    # The count value is the ordinal of the last enum entry
    if count < 0:
        count = 0
    # Determine how many entries to read
    num_entries = min(count + 1, len(expected_names)) if count < 1000 else min(count, len(expected_names))
    colors: dict[str, str] = {}
    actual_read = max(count + 1, num_entries) if count < 1000 else count
    for i in range(actual_read):
        name, pos = read_delphi_string(data, pos)
        _colon, pos = read_delphi_string(data, pos)
        value, pos = read_delphi_string(data, pos)
        if i < len(expected_names):
            colors[expected_names[i]] = convert_delphi_color(value)
    return colors, pos


def _read_font_table(data: bytes, pos: int) -> tuple[dict[str, dict], int]:
    """Read a Fonts section from the decompressed stream.

    Format: int32 count (enum ordinal), then triplets of (name, ':', font_spec).
    """
    count = struct.unpack_from("<i", data, pos)[0]
    pos += 4
    if count < 0:
        count = 0
    num_entries = min(count + 1, len(STYLE_FONT_NAMES)) if count < 1000 else min(count, len(STYLE_FONT_NAMES))
    fonts: dict[str, dict] = {}
    actual_read = max(count + 1, num_entries) if count < 1000 else count
    for i in range(actual_read):
        name, pos = read_delphi_string(data, pos)
        _colon, pos = read_delphi_string(data, pos)
        value, pos = read_delphi_string(data, pos)
        if i < len(STYLE_FONT_NAMES):
            fonts[STYLE_FONT_NAMES[i]] = parse_font_string(value)
    return fonts, pos


def parse_vsf(path: Path | str) -> dict[str, Any]:
    """Parse a VCL Style (.vsf) file and return structured theme data.

    Returns a dict with keys: name, version, author, author_email,
    author_url, colors, sys_colors, fonts, bitmaps (metadata only).
    """
    path = Path(path)
    raw = path.read_bytes()
    compressed = parse_vsf_header(raw)
    data = zlib.decompress(compressed)

    pos = 0
    # 1. Header strings
    name, pos = read_delphi_string(data, pos)
    version, pos = read_delphi_string(data, pos)
    author, pos = read_delphi_string(data, pos)
    author_email, pos = read_delphi_string(data, pos)
    author_url, pos = read_delphi_string(data, pos)

    # 2. Display names block (v1.0 has this as int64 size)
    display_names_size = struct.unpack_from("<q", data, pos)[0]
    pos += 8
    if display_names_size > 0:
        pos += display_names_size

    # 3. Bitmaps - skip for now, just record count and advance
    bitmap_count = struct.unpack_from("<i", data, pos)[0]
    pos += 4
    bitmap_info: list[dict[str, Any]] = []
    for i in range(bitmap_count):
        # TseBitmap.LoadFromStream: ReadString(name), int32 W, int32 H, W*H*4 bytes BGRA
        bmp_name, pos = read_delphi_string(data, pos)
        w = struct.unpack_from("<i", data, pos)[0]
        pos += 4
        h = struct.unpack_from("<i", data, pos)[0]
        pos += 4
        pixel_bytes = w * h * 4
        pos += pixel_bytes  # skip pixel data for now
        # Two boolean flags after each bitmap
        pos += 1  # transparent flag
        pos += 1  # alpha flag
        bitmap_info.append({"name": bmp_name, "width": w, "height": h, "index": i})

    # 4. Style objects - skip for now, just advance past them
    object_count = struct.unpack_from("<i", data, pos)[0]
    pos += 4
    for _ in range(object_count):
        # Each object: ReadString(className), uint32 size, size bytes of DFM data
        _class_name, pos = read_delphi_string(data, pos)
        obj_size = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        pos += obj_size  # skip DFM data for now

    # 5. Colors
    colors, pos = _read_color_table(data, pos, STYLE_COLOR_NAMES)

    # 6. SysColors
    sys_colors, pos = _read_color_table(data, pos, SYS_COLOR_NAMES)

    # 7. Fonts
    fonts, pos = _read_font_table(data, pos)

    return {
        "name": name,
        "version": version,
        "author": author,
        "author_email": author_email,
        "author_url": author_url,
        "bitmaps": bitmap_info,
        "colors": colors,
        "sys_colors": sys_colors,
        "fonts": fonts,
    }
```

- [ ] **Step 4: Run all tests**

```bash
PYTHONPATH=tools python -m pytest tests/test_vsf_reader.py -v
```

Expected: All tests PASS. The CODEX.vsf tests should find real color/font data.

- [ ] **Step 5: Verify parsed data looks correct**

```bash
PYTHONPATH=tools python -c "
from vsf_parser.vsf_reader import parse_vsf
import json
result = parse_vsf('installer/src/Include/Style/CODEX.vsf')
print(json.dumps({k: v for k, v in result.items() if k != 'bitmaps'}, indent=2))
"
```

Expected: JSON output with colors like `"ktcWindow": "#000000"`, fonts with Tahoma entries, sys_colors with `"clWindowText": "#e1e0e6"`.

- [ ] **Step 6: Commit**

```bash
git add tools/vsf_parser/vsf_reader.py tests/test_vsf_reader.py
git commit -m "feat: parse VSF metadata - colors, fonts, sys_colors"
```

---

### Task 4: Bitmap extraction

**Files:**
- Modify: `tools/vsf_parser/vsf_reader.py`
- Modify: `tests/test_vsf_reader.py`

- [ ] **Step 1: Write failing test for bitmap extraction**

Add to `tests/test_vsf_reader.py`:
```python
from PIL import Image


@pytest.mark.skipif(
    not (STYLE_DIR / "CODEX.vsf").exists(),
    reason="CODEX.vsf not found",
)
def test_parse_vsf_codex_bitmaps():
    result = parse_vsf(STYLE_DIR / "CODEX.vsf", extract_bitmaps=True)
    bitmaps = result["bitmaps"]
    assert len(bitmaps) >= 1
    # First bitmap should have actual image data
    first = bitmaps[0]
    assert "image" in first
    img = first["image"]
    assert isinstance(img, Image.Image)
    assert img.width > 0
    assert img.height > 0
```

- [ ] **Step 2: Run test to verify it fails**

```bash
PYTHONPATH=tools python -m pytest tests/test_vsf_reader.py::test_parse_vsf_codex_bitmaps -v
```

Expected: FAIL (parse_vsf doesn't accept extract_bitmaps parameter)

- [ ] **Step 3: Add bitmap extraction to parse_vsf**

Modify the bitmap parsing section of `parse_vsf` in `tools/vsf_parser/vsf_reader.py`. Replace the bitmap loop with:

```python
def _extract_bitmap(data: bytes, pos: int) -> tuple[dict[str, Any], int]:
    """Extract a single TseBitmap from the stream.

    Format: ReadString(name), int32 W, int32 H, W*H*4 bytes BGRA (bottom-up).
    Then 1 byte transparent flag, 1 byte alpha flag.
    """
    bmp_name, pos = read_delphi_string(data, pos)
    w = struct.unpack_from("<i", data, pos)[0]
    pos += 4
    h = struct.unpack_from("<i", data, pos)[0]
    pos += 4

    info: dict[str, Any] = {"name": bmp_name, "width": w, "height": h}

    pixel_bytes = w * h * 4
    if h > 0 and w > 0:
        # Raw BGRA pixel data, bottom-up scanline order (like BMP)
        raw_pixels = data[pos : pos + pixel_bytes]
        pos += pixel_bytes

        # Convert bottom-up BGRA to top-down RGBA for PIL
        from PIL import Image

        img = Image.frombytes("RGBA", (w, h), raw_pixels, "raw", "BGRA", 0, -1)
        info["image"] = img
    else:
        pos += pixel_bytes

    # Two boolean flags
    transparent = struct.unpack_from("<?", data, pos)[0]
    pos += 1
    alpha = struct.unpack_from("<?", data, pos)[0]
    pos += 1
    info["transparent"] = transparent
    info["alpha"] = alpha

    return info, pos
```

Then update the `parse_vsf` function signature and bitmap loop:

```python
def parse_vsf(path: Path | str, *, extract_bitmaps: bool = False) -> dict[str, Any]:
```

Replace the bitmap loop body with:
```python
    bitmap_info: list[dict[str, Any]] = []
    for i in range(bitmap_count):
        if extract_bitmaps:
            bmp, pos = _extract_bitmap(data, pos)
            bmp["index"] = i
            bitmap_info.append(bmp)
        else:
            # Skip bitmap data
            bmp_name, pos = read_delphi_string(data, pos)
            w = struct.unpack_from("<i", data, pos)[0]
            pos += 4
            h = struct.unpack_from("<i", data, pos)[0]
            pos += 4
            pos += w * h * 4  # pixel data
            pos += 1  # transparent flag
            pos += 1  # alpha flag
            bitmap_info.append({"name": bmp_name, "width": w, "height": h, "index": i})
```

- [ ] **Step 4: Run tests**

```bash
PYTHONPATH=tools python -m pytest tests/test_vsf_reader.py -v
```

Expected: All tests PASS including the new bitmap test.

- [ ] **Step 5: Visual verification**

```bash
PYTHONPATH=tools python -c "
from vsf_parser.vsf_reader import parse_vsf
result = parse_vsf('installer/src/Include/Style/CODEX.vsf', extract_bitmaps=True)
for bmp in result['bitmaps']:
    img = bmp.get('image')
    if img:
        img.save(f'/tmp/vsf_bitmap_{bmp[\"index\"]}_{bmp[\"name\"]}.png')
        print(f'Saved bitmap {bmp[\"index\"]}: {bmp[\"name\"]} ({img.width}x{img.height})')
"
```

Expected: Saves PNG files to /tmp. The main bitmap (style.png) should be visible as a dark-themed atlas with button/checkbox/scrollbar graphics.

- [ ] **Step 6: Commit**

```bash
git add tools/vsf_parser/vsf_reader.py tests/test_vsf_reader.py
git commit -m "feat: extract bitmaps from VSF as PIL Images"
```

---

### Task 5: DFM binary parser for style objects

**Files:**
- Create: `tools/vsf_parser/delphi_dfm.py`
- Create: `tests/test_delphi_dfm.py`

- [ ] **Step 1: Write failing test for DFM binary parsing**

The style objects are serialized using Delphi's `WriteComponent` which produces TPF0 binary format. Each object in the VSF stream is: `ReadString(className)`, `uint32 size`, then `size` bytes of DFM data.

The DFM binary format starts with `TPF0` signature, then: class name (string), component name (string), properties list, terminated by a zero byte. Properties: name (string), type byte, value. Nested objects start with type bytes for child components.

Create `tests/test_delphi_dfm.py`:
```python
import struct
import pytest
from vsf_parser.delphi_dfm import parse_dfm_binary


def test_parse_dfm_empty_object():
    """TPF0 + class name + instance name + no properties + end marker."""
    data = b"TPF0"
    data += b"\x0BTSeStyleObj"  # class name (length-prefixed byte)
    data += b"\x04Test"         # instance name
    data += b"\x00"             # end of properties
    data += b"\x00"             # end of object
    result = parse_dfm_binary(data)
    assert result["_class"] == "TSeStyleObj"
    assert result["_name"] == "Test"


def test_parse_dfm_integer_property():
    data = b"TPF0"
    data += b"\x0BTSeStyleObj"
    data += b"\x04Test"
    # Property: "Left" = 10 (int16)
    data += b"\x04Left"         # property name
    data += b"\x03"             # type: int16
    data += struct.pack("<h", 10)
    data += b"\x00"             # end of properties
    data += b"\x00"             # end of object
    result = parse_dfm_binary(data)
    assert result["Left"] == 10


def test_parse_dfm_string_property():
    data = b"TPF0"
    data += b"\x0BTSeStyleObj"
    data += b"\x04Test"
    # Property: "Name" = "style.png" (string type 0x06)
    data += b"\x04Name"
    data += b"\x06"                    # type: string
    data += b"\x09style.png"           # length-prefixed string
    data += b"\x00"                    # end of properties
    data += b"\x00"                    # end of object
    result = parse_dfm_binary(data)
    assert result["Name"] == "style.png"
```

- [ ] **Step 2: Run test to verify it fails**

```bash
PYTHONPATH=tools python -m pytest tests/test_delphi_dfm.py -v
```

Expected: FAIL with `ModuleNotFoundError`

- [ ] **Step 3: Implement DFM binary parser**

Create `tools/vsf_parser/delphi_dfm.py`:
```python
"""Delphi DFM binary (TPF0) format parser.

Parses the binary component stream format used by Delphi's
TStream.WriteComponent/ReadComponent methods.

TPF0 format:
  - 4 bytes: "TPF0" signature
  - Class name: length-prefixed string (1-byte length)
  - Instance name: length-prefixed string (1-byte length)
  - Properties: sequence of (name, type, value) until name is empty
  - Children: nested TPF0 objects until end marker

Property types:
  0x00 = end of properties / end of object
  0x01 = list begin
  0x02 = int8
  0x03 = int16
  0x04 = int32
  0x05 = double (extended in Delphi, 10 bytes; or 8-byte double)
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
        # List - read items until end marker
        items = []
        while True:
            item_type = data[pos]
            if item_type == 0x00:
                pos += 1
                break
            value, pos = _read_property_value(data, pos, item_type)
            items.append(value)
        return items, pos

    if type_byte == 0x02:
        # int8
        value = struct.unpack_from("<b", data, pos)[0]
        return value, pos + 1

    if type_byte == 0x03:
        # int16
        value = struct.unpack_from("<h", data, pos)[0]
        return value, pos + 2

    if type_byte == 0x04:
        # int32
        value = struct.unpack_from("<i", data, pos)[0]
        return value, pos + 4

    if type_byte == 0x05:
        # Extended float (Delphi uses 10-byte extended, but may be 8-byte double)
        # Try 10-byte first, fall back to 8-byte
        try:
            value = struct.unpack_from("<d", data, pos)[0]
            return value, pos + 10  # Delphi extended is 10 bytes
        except struct.error:
            return 0.0, pos + 8

    if type_byte == 0x06:
        # Short string
        return _read_short_string(data, pos)

    if type_byte == 0x07:
        # Ident/enum - same encoding as short string
        return _read_short_string(data, pos)

    if type_byte == 0x08:
        # False
        return False, pos

    if type_byte == 0x09:
        # True
        return True, pos

    if type_byte == 0x0A:
        # Binary data
        length = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        value = data[pos : pos + length]
        return value, pos + length

    if type_byte == 0x0B:
        # Set - sequence of ident strings until empty
        items = []
        while True:
            s, pos = _read_short_string(data, pos)
            if s == "":
                break
            items.append(s)
        return items, pos

    if type_byte == 0x0C:
        # Long string (4-byte length)
        length = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        value = data[pos : pos + length].decode("ascii", errors="replace")
        return value, pos + length

    if type_byte == 0x0D:
        # Nil
        return None, pos

    if type_byte == 0x12:
        # UTF-8 string (4-byte length)
        length = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        value = data[pos : pos + length].decode("utf-8", errors="replace")
        return value, pos + length

    if type_byte == 0x14:
        # int64
        value = struct.unpack_from("<q", data, pos)[0]
        return value, pos + 8

    # Unknown type - cannot continue safely
    raise ValueError(f"Unknown DFM property type: 0x{type_byte:02x} at pos {pos}")


def _parse_object(data: bytes, pos: int) -> tuple[dict[str, Any], int]:
    """Parse a single DFM object (class + name + properties + children)."""
    class_name, pos = _read_short_string(data, pos)
    instance_name, pos = _read_short_string(data, pos)

    obj: dict[str, Any] = {
        "_class": class_name,
        "_name": instance_name,
    }

    # Read properties until empty name
    while True:
        prop_name_len = data[pos]
        if prop_name_len == 0x00:
            pos += 1
            break
        prop_name, pos = _read_short_string(data, pos)
        type_byte = data[pos]
        pos += 1
        value, pos = _read_property_value(data, pos, type_byte)
        obj[prop_name] = value

    # Read child objects until end marker (0x00)
    children = []
    while pos < len(data) and data[pos] != 0x00:
        if data[pos:pos+4] == b"TPF0":
            pos += 4  # skip TPF0 of nested object
            child, pos = _parse_object(data, pos)
            children.append(child)
        else:
            # Assume it's the class inheritance byte for nested component
            # In DFM binary, nested components don't repeat TPF0
            child, pos = _parse_object(data, pos)
            children.append(child)

    if pos < len(data) and data[pos] == 0x00:
        pos += 1  # consume end marker

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
```

- [ ] **Step 4: Run tests**

```bash
PYTHONPATH=tools python -m pytest tests/test_delphi_dfm.py -v
```

Expected: All 3 tests PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/vsf_parser/delphi_dfm.py tests/test_delphi_dfm.py
git commit -m "feat: DFM binary (TPF0) format parser"
```

---

### Task 6: Parse style objects from VSF

**Files:**
- Modify: `tools/vsf_parser/vsf_reader.py`
- Modify: `tests/test_vsf_reader.py`

- [ ] **Step 1: Write failing test**

Add to `tests/test_vsf_reader.py`:
```python
@pytest.mark.skipif(
    not (STYLE_DIR / "CODEX.vsf").exists(),
    reason="CODEX.vsf not found",
)
def test_parse_vsf_codex_objects():
    result = parse_vsf(STYLE_DIR / "CODEX.vsf", extract_objects=True)
    objects = result["objects"]
    assert len(objects) > 0
    # Should have a 'Form' object
    form_obj = None
    for obj in objects:
        if obj.get("_name", "").lower() == "form" or obj.get("_class", "") == "TSeStyleObject":
            form_obj = obj
            break
    assert form_obj is not None, f"No form object found. Objects: {[o.get('_name') for o in objects]}"
```

- [ ] **Step 2: Run test to verify it fails**

```bash
PYTHONPATH=tools python -m pytest tests/test_vsf_reader.py::test_parse_vsf_codex_objects -v
```

Expected: FAIL (extract_objects not supported)

- [ ] **Step 3: Add object parsing to parse_vsf**

Add `extract_objects` parameter to `parse_vsf`:

```python
def parse_vsf(path: Path | str, *, extract_bitmaps: bool = False, extract_objects: bool = False) -> dict[str, Any]:
```

Replace the style objects skip-loop with:
```python
    # 4. Style objects
    object_count = struct.unpack_from("<i", data, pos)[0]
    pos += 4
    style_objects: list[dict[str, Any]] = []
    for _ in range(object_count):
        class_name, pos = read_delphi_string(data, pos)
        obj_size = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        if extract_objects:
            obj_data = data[pos : pos + obj_size]
            try:
                from .delphi_dfm import parse_dfm_binary
                obj = parse_dfm_binary(obj_data)
                obj["_style_class"] = class_name
                style_objects.append(obj)
            except Exception as e:
                style_objects.append({
                    "_style_class": class_name,
                    "_parse_error": str(e),
                    "_raw_size": obj_size,
                })
        pos += obj_size
```

And add to the return dict:
```python
    result = {
        "name": name,
        ...
        "objects": style_objects if extract_objects else [],
    }
```

- [ ] **Step 4: Run all tests**

```bash
PYTHONPATH=tools python -m pytest tests/test_vsf_reader.py -v
```

Expected: All tests PASS.

- [ ] **Step 5: Verify object tree**

```bash
PYTHONPATH=tools python -c "
from vsf_parser.vsf_reader import parse_vsf
import json
result = parse_vsf('installer/src/Include/Style/CODEX.vsf', extract_objects=True)
for obj in result['objects']:
    name = obj.get('_name', '?')
    cls = obj.get('_style_class', obj.get('_class', '?'))
    children = len(obj.get('_children', []))
    err = obj.get('_parse_error', '')
    status = f' ERROR: {err}' if err else ''
    print(f'{cls}: {name} ({children} children){status}')
"
```

Expected: A list of style objects like Form, Button, Edit, CheckBox, etc.

- [ ] **Step 6: Commit**

```bash
git add tools/vsf_parser/vsf_reader.py tests/test_vsf_reader.py
git commit -m "feat: parse style objects from VSF using DFM parser"
```

---

### Task 7: JSON output and CLI

**Files:**
- Create: `tools/vsf_parser/cli.py`
- Modify: `tools/vsf_parser/__main__.py`

- [ ] **Step 1: Implement CLI**

Create `tools/vsf_parser/cli.py`:
```python
"""CLI interface for the VSF parser."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from .vsf_reader import parse_vsf


def _serialize_result(result: dict) -> dict:
    """Make parse result JSON-serializable by removing PIL images."""
    clean = {}
    for key, value in result.items():
        if key == "bitmaps":
            clean[key] = [
                {k: v for k, v in bmp.items() if k != "image"}
                for bmp in value
            ]
        elif key == "objects":
            clean[key] = _clean_objects(value)
        else:
            clean[key] = value
    return clean


def _clean_objects(objects: list) -> list:
    """Remove non-serializable data from style objects."""
    cleaned = []
    for obj in objects:
        clean_obj = {}
        for k, v in obj.items():
            if isinstance(v, bytes):
                clean_obj[k] = f"<binary {len(v)} bytes>"
            elif isinstance(v, list) and k == "_children":
                clean_obj[k] = _clean_objects(v)
            else:
                clean_obj[k] = v
        cleaned.append(clean_obj)
    return cleaned


def cmd_parse(args: argparse.Namespace) -> None:
    """Parse a single VSF file."""
    vsf_path = Path(args.file)
    if not vsf_path.exists():
        print(f"Error: {vsf_path} not found", file=sys.stderr)
        sys.exit(1)

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    result = parse_vsf(
        vsf_path,
        extract_bitmaps=not args.json_only,
        extract_objects=True,
    )

    # Save JSON
    json_path = output_dir / "theme.json"
    serializable = _serialize_result(result)
    json_path.write_text(json.dumps(serializable, indent=2))
    print(f"Wrote {json_path}")

    # Save bitmaps as PNGs
    if not args.json_only:
        assets_dir = output_dir / "assets"
        assets_dir.mkdir(exist_ok=True)
        for bmp in result["bitmaps"]:
            img = bmp.get("image")
            if img:
                png_path = assets_dir / f"{bmp['name']}"
                if not png_path.suffix:
                    png_path = png_path.with_suffix(".png")
                img.save(png_path)
                print(f"Wrote {png_path} ({img.width}x{img.height})")

    if not args.json_only:
        # Generate QSS
        from .qss_generator import generate_qss

        qss_path = output_dir / "style.qss"
        qss_content = generate_qss(serializable, output_dir / "assets")
        qss_path.write_text(qss_content)
        print(f"Wrote {qss_path}")


def cmd_parse_all(args: argparse.Namespace) -> None:
    """Parse all VSF files in a directory."""
    style_dir = Path(args.directory)
    output_dir = Path(args.output)

    vsf_files = sorted(style_dir.glob("*.vsf"))
    if not vsf_files:
        print(f"No .vsf files found in {style_dir}", file=sys.stderr)
        sys.exit(1)

    for vsf_path in vsf_files:
        theme_name = vsf_path.stem
        theme_dir = output_dir / theme_name
        print(f"\n=== Parsing {vsf_path.name} -> {theme_dir} ===")
        # Reuse cmd_parse logic
        ns = argparse.Namespace(
            file=str(vsf_path),
            output=str(theme_dir),
            json_only=args.json_only,
        )
        try:
            cmd_parse(ns)
        except Exception as e:
            print(f"Error parsing {vsf_path.name}: {e}", file=sys.stderr)


def main() -> None:
    parser = argparse.ArgumentParser(
        prog="vsf_parser",
        description="Parse Delphi VCL Style (.vsf) files",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    # parse command
    p_parse = subparsers.add_parser("parse", help="Parse a single VSF file")
    p_parse.add_argument("file", help="Path to .vsf file")
    p_parse.add_argument("-o", "--output", required=True, help="Output directory")
    p_parse.add_argument("--json-only", action="store_true", help="Skip QSS/PNG generation")
    p_parse.set_defaults(func=cmd_parse)

    # parse-all command
    p_all = subparsers.add_parser("parse-all", help="Parse all VSF files in a directory")
    p_all.add_argument("directory", help="Directory containing .vsf files")
    p_all.add_argument("-o", "--output", required=True, help="Output directory")
    p_all.add_argument("--json-only", action="store_true", help="Skip QSS/PNG generation")
    p_all.set_defaults(func=cmd_parse_all)

    args = parser.parse_args()
    args.func(args)
```

- [ ] **Step 2: Update __main__.py**

Already correct from Task 1.

- [ ] **Step 3: Test JSON-only mode**

```bash
source tools/vsf_parser/.venv/bin/activate
PYTHONPATH=tools python -m vsf_parser parse installer/src/Include/Style/CODEX.vsf -o /tmp/vsf_test/CODEX --json-only
cat /tmp/vsf_test/CODEX/theme.json | head -50
```

Expected: JSON file written with colors, fonts, metadata.

- [ ] **Step 4: Commit**

```bash
git add tools/vsf_parser/cli.py tools/vsf_parser/__main__.py
git commit -m "feat: CLI for vsf_parser with parse and parse-all commands"
```

---

### Task 8: QSS generator

**Files:**
- Create: `tools/vsf_parser/qss_generator.py`
- Create: `tests/test_qss_generator.py`

- [ ] **Step 1: Write failing test**

Create `tests/test_qss_generator.py`:
```python
import pytest
from vsf_parser.qss_generator import generate_qss


def _make_theme_data() -> dict:
    return {
        "name": "TestTheme",
        "colors": {
            "ktcWindow": "#000000",
            "ktcButton": "#353739",
            "ktcButtonHot": "#e86625",
            "ktcButtonPressed": "#e86625",
            "ktcButtonFocused": "#e86625",
            "ktcButtonDisabled": "#1b1c1d",
            "ktcEdit": "#0c0c0c",
            "ktcEditDisabled": "#131111",
            "ktcComboBox": "#1b1c1d",
            "ktcPanel": "#000000",
            "ktcBorder": "#1a1b1c",
            "ktcListBox": "#1b1c1d",
            "ktcTreeView": "#3c3c3c",
        },
        "sys_colors": {
            "clWindowText": "#e1e0e6",
            "clBtnText": "#c0c0c0",
            "clHighlight": "#e86625",
            "clBtnFace": "#000000",
        },
        "fonts": {
            "ktfButtonTextNormal": {"name": "Tahoma", "size": 8, "color": "#e9e9e9", "style": ""},
            "ktfButtonTextHot": {"name": "Tahoma", "size": 8, "color": "#e9e9e9", "style": ""},
            "ktfButtonTextPressed": {"name": "Tahoma", "size": 8, "color": "#e9e9e9", "style": ""},
            "ktfButtonTextDisabled": {"name": "Tahoma", "size": 8, "color": "#3c3c3c", "style": ""},
            "ktfStaticTextNormal": {"name": "Tahoma", "size": 8, "color": "#c0c0c0", "style": ""},
            "ktfCheckBoxTextNormal": {"name": "Tahoma", "size": 8, "color": "#c0c0c0", "style": ""},
            "ktfEditBoxTextNormal": {"name": "Tahoma", "size": 8, "color": "#e9e9e9", "style": ""},
            "ktfEditBoxTextDisabled": {"name": "Tahoma", "size": 8, "color": "#3c3c3c", "style": ""},
        },
    }


def test_generate_qss_contains_widget_background():
    qss = generate_qss(_make_theme_data(), None)
    assert "QWidget" in qss
    assert "#000000" in qss


def test_generate_qss_contains_button_states():
    qss = generate_qss(_make_theme_data(), None)
    assert "QPushButton" in qss
    assert "QPushButton:hover" in qss
    assert "QPushButton:pressed" in qss
    assert "QPushButton:disabled" in qss


def test_generate_qss_contains_edit():
    qss = generate_qss(_make_theme_data(), None)
    assert "QLineEdit" in qss


def test_generate_qss_contains_checkbox():
    qss = generate_qss(_make_theme_data(), None)
    assert "QCheckBox" in qss
```

- [ ] **Step 2: Run test to verify it fails**

```bash
PYTHONPATH=tools python -m pytest tests/test_qss_generator.py -v
```

Expected: FAIL with `ModuleNotFoundError`

- [ ] **Step 3: Implement QSS generator**

Create `tools/vsf_parser/qss_generator.py`:
```python
"""Generate Qt6 QSS stylesheets from parsed VSF theme data."""

from __future__ import annotations

from pathlib import Path
from typing import Any


def _font_css(font: dict[str, Any]) -> str:
    """Convert a parsed font dict to CSS font properties."""
    parts = []
    size = font.get("size", 8)
    parts.append(f"font-size: {size}pt;")
    name = font.get("name", "Tahoma")
    parts.append(f'font-family: "{name}";')
    color = font.get("color", "#ffffff")
    parts.append(f"color: {color};")
    style = font.get("style", "")
    if style == "bold":
        parts.append("font-weight: bold;")
    return "\n    ".join(parts)


def _get_color(data: dict, key: str, fallback: str = "#000000") -> str:
    """Get a color from the theme data, checking colors then sys_colors."""
    colors = data.get("colors", {})
    if key in colors:
        return colors[key]
    sys_colors = data.get("sys_colors", {})
    if key in sys_colors:
        return sys_colors[key]
    return fallback


def _get_font(data: dict, key: str) -> dict[str, Any] | None:
    """Get a font from the theme data."""
    return data.get("fonts", {}).get(key)


def generate_qss(theme_data: dict[str, Any], assets_dir: Path | None) -> str:
    """Generate a Qt6 QSS stylesheet from parsed VSF theme data.

    Args:
        theme_data: Parsed theme dict with colors, fonts, sys_colors.
        assets_dir: Path to pre-sliced PNG assets (or None to skip image refs).

    Returns:
        QSS stylesheet as a string.
    """
    sections: list[str] = []
    sections.append(f"/* Auto-generated from VCL Style: {theme_data.get('name', 'Unknown')} */")
    sections.append("")

    # --- QWidget (form background) ---
    window_color = _get_color(theme_data, "ktcWindow")
    window_text = _get_color(theme_data, "clWindowText", "#e0e0e0")
    sections.append(f"""QWidget {{
    background-color: {window_color};
    color: {window_text};
}}""")

    # --- QPushButton ---
    btn_bg = _get_color(theme_data, "ktcButton", "#353739")
    btn_bg_hover = _get_color(theme_data, "ktcButtonHot", "#4a4a4a")
    btn_bg_pressed = _get_color(theme_data, "ktcButtonPressed", "#2a2a2a")
    btn_bg_focused = _get_color(theme_data, "ktcButtonFocused", "#4a4a4a")
    btn_bg_disabled = _get_color(theme_data, "ktcButtonDisabled", "#1d1c1b")
    btn_border = _get_color(theme_data, "ktcBorder", "#1a1a1a")
    btn_font = _get_font(theme_data, "ktfButtonTextNormal")
    btn_font_hover = _get_font(theme_data, "ktfButtonTextHot")
    btn_font_pressed = _get_font(theme_data, "ktfButtonTextPressed")
    btn_font_disabled = _get_font(theme_data, "ktfButtonTextDisabled")

    btn_css = f"background-color: {btn_bg};\n    border: 1px solid {btn_border};"
    if btn_font:
        btn_css += f"\n    {_font_css(btn_font)}"

    sections.append(f"""QPushButton {{
    {btn_css}
    padding: 4px 12px;
}}""")

    if btn_font_hover:
        sections.append(f"""QPushButton:hover {{
    background-color: {btn_bg_hover};
    {_font_css(btn_font_hover)}
}}""")
    else:
        sections.append(f"""QPushButton:hover {{
    background-color: {btn_bg_hover};
}}""")

    sections.append(f"""QPushButton:pressed {{
    background-color: {btn_bg_pressed};
    {_font_css(btn_font_pressed) if btn_font_pressed else ''}
}}""")

    sections.append(f"""QPushButton:focus {{
    background-color: {btn_bg_focused};
    border: 1px solid {btn_bg_focused};
}}""")

    sections.append(f"""QPushButton:disabled {{
    background-color: {btn_bg_disabled};
    {_font_css(btn_font_disabled) if btn_font_disabled else ''}
}}""")

    # --- QLineEdit ---
    edit_bg = _get_color(theme_data, "ktcEdit", "#0c0c0c")
    edit_bg_disabled = _get_color(theme_data, "ktcEditDisabled", "#111113")
    edit_font = _get_font(theme_data, "ktfEditBoxTextNormal")
    edit_font_disabled = _get_font(theme_data, "ktfEditBoxTextDisabled")

    sections.append(f"""QLineEdit {{
    background-color: {edit_bg};
    border: 1px solid {btn_border};
    padding: 2px 4px;
    {_font_css(edit_font) if edit_font else ''}
}}""")

    sections.append(f"""QLineEdit:disabled {{
    background-color: {edit_bg_disabled};
    {_font_css(edit_font_disabled) if edit_font_disabled else ''}
}}""")

    # --- QComboBox ---
    combo_bg = _get_color(theme_data, "ktcComboBox", "#1d1c1b")
    sections.append(f"""QComboBox {{
    background-color: {combo_bg};
    border: 1px solid {btn_border};
    padding: 2px 4px;
    {_font_css(edit_font) if edit_font else ''}
}}""")

    sections.append(f"""QComboBox QAbstractItemView {{
    background-color: {combo_bg};
    border: 1px solid {btn_border};
    selection-background-color: {btn_bg_hover};
}}""")

    # --- QCheckBox ---
    chk_font = _get_font(theme_data, "ktfCheckBoxTextNormal")
    sections.append(f"""QCheckBox {{
    spacing: 5px;
    {_font_css(chk_font) if chk_font else ''}
}}""")

    # --- QProgressBar ---
    highlight = _get_color(theme_data, "clHighlight", "#2566e8")
    sections.append(f"""QProgressBar {{
    background-color: {edit_bg};
    border: 1px solid {btn_border};
    text-align: center;
    {_font_css(edit_font) if edit_font else ''}
}}""")

    sections.append(f"""QProgressBar::chunk {{
    background-color: {highlight};
}}""")

    # --- QTextEdit / QPlainTextEdit (memo) ---
    sections.append(f"""QTextEdit, QPlainTextEdit {{
    background-color: {edit_bg};
    border: 1px solid {btn_border};
    {_font_css(edit_font) if edit_font else ''}
}}""")

    # --- QScrollBar ---
    scrollbar_bg = _get_color(theme_data, "clScrollBar", "#1d1c1b")
    sections.append(f"""QScrollBar:vertical {{
    background-color: {scrollbar_bg};
    width: 14px;
    border: none;
}}""")

    sections.append(f"""QScrollBar::handle:vertical {{
    background-color: {btn_bg};
    min-height: 20px;
    border-radius: 2px;
    margin: 2px;
}}""")

    sections.append(f"""QScrollBar::handle:vertical:hover {{
    background-color: {btn_bg_hover};
}}""")

    sections.append(f"""QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
    height: 0px;
}}""")

    # --- QTreeView / QListView ---
    tree_bg = _get_color(theme_data, "ktcTreeView", "#3c3c3c")
    listbox_bg = _get_color(theme_data, "ktcListBox", "#1d1c1b")
    sections.append(f"""QTreeView, QListView {{
    background-color: {listbox_bg};
    border: 1px solid {btn_border};
    alternate-background-color: {tree_bg};
}}""")

    sections.append(f"""QTreeView::item:selected, QListView::item:selected {{
    background-color: {highlight};
}}""")

    # --- QLabel ---
    label_font = _get_font(theme_data, "ktfStaticTextNormal")
    sections.append(f"""QLabel {{
    background: transparent;
    {_font_css(label_font) if label_font else ''}
}}""")

    # --- QGroupBox ---
    panel_bg = _get_color(theme_data, "ktcPanel")
    sections.append(f"""QGroupBox {{
    background-color: {panel_bg};
    border: 1px solid {btn_border};
    margin-top: 8px;
    padding-top: 8px;
}}""")

    return "\n\n".join(sections) + "\n"
```

- [ ] **Step 4: Run tests**

```bash
PYTHONPATH=tools python -m pytest tests/test_qss_generator.py -v
```

Expected: All 4 tests PASS.

- [ ] **Step 5: End-to-end test with real VSF**

```bash
source tools/vsf_parser/.venv/bin/activate
PYTHONPATH=tools python -m vsf_parser parse installer/src/Include/Style/CODEX.vsf -o /tmp/vsf_test/CODEX
cat /tmp/vsf_test/CODEX/style.qss
ls /tmp/vsf_test/CODEX/assets/
```

Expected: QSS file with dark theme colors, PNG bitmap assets in assets dir.

- [ ] **Step 6: Commit**

```bash
git add tools/vsf_parser/qss_generator.py tests/test_qss_generator.py
git commit -m "feat: QSS generator from parsed VSF theme data"
```

---

### Task 9: Parse all 18 themes end-to-end

**Files:**
- No new files

- [ ] **Step 1: Run parse-all on all themes**

```bash
source tools/vsf_parser/.venv/bin/activate
PYTHONPATH=tools python -m vsf_parser parse-all installer/src/Include/Style/ -o /tmp/vsf_all_themes/
```

Expected: All 18 themes parse successfully, each producing theme.json + style.qss + assets/.

- [ ] **Step 2: Spot-check a few themes**

```bash
# Check CODEX colors
python3 -c "import json; d=json.load(open('/tmp/vsf_all_themes/CODEX/theme.json')); print('CODEX window:', d['colors'].get('ktcWindow')); print('CODEX btn:', d['colors'].get('ktcButton'))"

# Check PLAZA colors (should be different from CODEX)
python3 -c "import json; d=json.load(open('/tmp/vsf_all_themes/PLAZA/theme.json')); print('PLAZA window:', d['colors'].get('ktcWindow')); print('PLAZA btn:', d['colors'].get('ktcButton'))"

# Check a Graphite theme
python3 -c "import json; d=json.load(open('/tmp/vsf_all_themes/AquaGraphite/theme.json')); print('AquaGraphite window:', d['colors'].get('ktcWindow')); print('AquaGraphite btn:', d['colors'].get('ktcButton'))"
```

Expected: Different color values for each theme.

- [ ] **Step 3: Run full test suite**

```bash
PYTHONPATH=tools python -m pytest tests/ -v
```

Expected: All tests PASS.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "feat: vsf_parser complete - parses all 18 VCL themes"
```
