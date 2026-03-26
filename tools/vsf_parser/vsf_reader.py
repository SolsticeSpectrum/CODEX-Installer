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
        "name": parts[0].strip() if len(parts) > 0 else "Tahoma",
        "size": int(parts[1].strip()) if len(parts) > 1 else 8,
        "charset": int(parts[2].strip()) if len(parts) > 2 else 1,
        "color": "#000000",
        "style": "",
    }
    if len(parts) >= 6:
        r = int(parts[3].strip())
        g = int(parts[4].strip())
        b = int(parts[5].strip())
        result["color"] = f"#{r:02x}{g:02x}{b:02x}"
    if len(parts) >= 7:
        result["style"] = parts[6].strip()
    return result


def _read_color_table(
    data: bytes, pos: int, expected_names: list[str]
) -> tuple[dict[str, str], int]:
    """Read a Colors section from the decompressed stream.

    The count is a 1-byte Delphi enum ordinal (High(TSeStyleColor)).
    Number of entries = ordinal + 1 (enums start at 0).
    Each entry is a triplet: (name_string, ':', color_string).
    """
    raw_count = data[pos]
    pos += 1
    num_entries = raw_count + 1
    colors: dict[str, str] = {}
    for i in range(num_entries):
        name, pos = read_delphi_string(data, pos)
        _colon, pos = read_delphi_string(data, pos)
        value, pos = read_delphi_string(data, pos)
        if i < len(expected_names):
            colors[expected_names[i]] = convert_delphi_color(value)
    return colors, pos


def _read_font_table(data: bytes, pos: int) -> tuple[dict[str, dict], int]:
    """Read a Fonts section from the decompressed stream.

    Count is a 1-byte Delphi enum ordinal (High(TSeStyleFont)).
    """
    raw_count = data[pos]
    pos += 1
    num_entries = raw_count + 1
    fonts: dict[str, dict] = {}
    for i in range(num_entries):
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

    # 2. Display names block (int64 size, then that many bytes)
    display_names_size = struct.unpack_from("<q", data, pos)[0]
    pos += 8
    if display_names_size > 0:
        pos += display_names_size

    # 3. Bitmaps - skip pixel data, record metadata
    bitmap_count = struct.unpack_from("<i", data, pos)[0]
    pos += 4
    bitmap_info: list[dict[str, Any]] = []
    for i in range(bitmap_count):
        # TseBitmap: ReadString(name), int32 W, int32 H, W*H*4 BGRA pixels
        bmp_name, pos = read_delphi_string(data, pos)
        w = struct.unpack_from("<i", data, pos)[0]
        pos += 4
        h = struct.unpack_from("<i", data, pos)[0]
        pos += 4
        pos += w * h * 4  # skip pixel data
        pos += 1  # transparent flag
        pos += 1  # alpha flag
        bitmap_info.append({"name": bmp_name, "width": w, "height": h, "index": i})

    # 4. Style objects - skip DFM data for now
    object_count = struct.unpack_from("<i", data, pos)[0]
    pos += 4
    for _ in range(object_count):
        _class_name, pos = read_delphi_string(data, pos)
        obj_size = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        pos += obj_size

    # 5. Colors
    colors, pos = _read_color_table(data, pos, STYLE_COLOR_NAMES)

    # 6. SysColors (uses int count, not enum ordinal)
    sys_count = struct.unpack_from("<i", data, pos)[0]
    pos += 4
    sys_colors: dict[str, str] = {}
    for i in range(sys_count):
        sc_name, pos = read_delphi_string(data, pos)
        _colon, pos = read_delphi_string(data, pos)
        sc_value, pos = read_delphi_string(data, pos)
        if i < len(SYS_COLOR_NAMES):
            sys_colors[SYS_COLOR_NAMES[i]] = convert_delphi_color(sc_value)

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


def parse_vsf_header(data: bytes) -> bytes:
    """Validate VSF header and return the compressed payload.

    Raises ValueError if header doesn't match VCL_STYLE 1.0.
    """
    if data[:VSF_HEADER_LEN] != VSF_HEADER:
        raise ValueError(
            f"Not a VCL_STYLE 1.0 file (got {data[:VSF_HEADER_LEN]!r})"
        )
    return data[VSF_HEADER_LEN:]
