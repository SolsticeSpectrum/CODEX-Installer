# VSF Parser Tool - Design Spec

## Goal

Parse Delphi VCL Style (.vsf) files on Linux and produce JSON theme data + pre-sliced PNG assets + generated Qt6 QSS stylesheets, enabling pixel-perfect replication of the CODEX-Installer's dark themed UI in a native Linux C++/Qt6 application.

## Context

The CODEX-Installer is a Windows Inno Setup installer that uses VclStylesInno.dll to load .vsf theme files and apply dark themes to the installer UI. The DLL only supports `VCL_STYLE 1.0` format. All 18 included .vsf files use this format. This tool is the first sub-project of a larger effort to port the installer to Linux.

## VSF 1.0 Binary Format

Derived from Embarcadero's `StyleAPI.inc` and `StyleUtils.inc` source code.

### File Layout

```
[13 bytes] ASCII header: "VCL_STYLE 1.0"
[rest]     zlib-compressed stream containing:
```

### Decompressed Stream (sequential reads)

1. **Header strings** (5x length-prefixed UTF-16LE):
   - Name, Version, Author, AuthorEMail, AuthorURL
   - String format: int32 char_count + (char_count * 2) bytes UTF-16LE

2. **Display names block**:
   - int64 size; if > 0, skip `size` bytes

3. **Bitmaps**:
   - int32 count
   - For each: TBitmap stream data + 1 byte (transparent flag) + 1 byte (alpha flag)
   - TBitmap stream: Delphi's TBitmap.LoadFromStream format (type byte + BMP data)

4. **Style objects**:
   - int32 count
   - For each: recursive binary object tree (see LoadStyleObjectBinary)
   - Each object has: class name, properties (Delphi DFM binary format), child objects
   - Objects contain BitmapLink references with Name + Rect for atlas slicing

5. **Colors**:
   - TSeStyleColor enum count (as int32)
   - For each: ReadString(name), ReadString(":"), ReadString(color_value)
   - Color values are Delphi color strings: "$00BBGGRR" or "clBlack", "clSilver", etc.

6. **SysColors**:
   - int32 count (up to 23)
   - Same triplet format as Colors
   - Maps Windows system colors (clBtnFace, clWindow, clWindowText, etc.)

7. **Fonts**:
   - TSeStyleFont enum count (as int32)
   - For each: ReadString(name), ReadString(":"), ReadString(font_spec)
   - Font spec format: "FontName,Size,Charset,R,G,B[,style]"

## Color Enums (TSeStyleColor)

In order: ktcBorder, ktcCategoryButtons, ktcCategoryPanelGroup, ktcComboBox, ktcComboBoxDisabled, ktcButton, ktcButtonHot, ktcButtonPressed, ktcButtonFocused, ktcButtonDisabled, ktcEdit, ktcEditDisabled, ktcGrid, ktcGenericGradientBase, ktcGenericGradientEnd, ktcHintGradientBase, ktcListBox, ktcListBoxDisabled, ktcListView, ktcPanel, ktcPanelDisabled, ktcTreeView, ktcWindow, ktcSplitter, ktcCategoryButtonsGradientBase, ktcCategoryButtonsGradientEnd, ktcToolBarGradientBase, ktcToolBarGradientEnd, ktcGenericBackground, ktcHintGradientEnd

## SysColor Names (23 entries)

clActiveBorder, clActiveCaption, clBtnFace, clBtnHighlight, clBtnShadow, clBtnText, clCaptionText, clGrayText, clHighlight, clHighlightText, clInactiveBorder, clInactiveCaption, clInactiveCaptionText, clInfoBk, clInfoText, clMenu, clMenuText, clScrollBar, cl3DDkShadow, cl3DLight, clWindow, clWindowFrame, clWindowText

## Font Enums (TSeStyleFont)

Starting from: ktfCaptionTextNormal, ktfCaptionTextInactive, ktfSmCaptionTextNormal, ktfSmCaptionTextInactive, ktfStaticTextNormal, ktfStaticTextHot, ktfStaticTextFocused, ktfStaticTextDisabled, ktfPanelTextNormal, ktfPanelTextDisabled, ktfButtonTextNormal, ktfButtonTextPressed, ktfButtonTextHot, ktfButtonTextFocused, ktfButtonTextDisabled, ktfCheckBoxTextNormal, ktfCheckBoxTextPressed, ktfCheckBoxTextHot, ktfCheckBoxTextFocused, ktfCheckBoxTextDisabled, ktfRadioButtonTextNormal, ktfRadioButtonTextPressed, ktfRadioButtonTextHot, ktfRadioButtonTextFocused, ktfRadioButtonTextDisabled, ktfGroupBoxTextNormal, ktfGroupBoxTextDisabled, ktfWindowTextNormal, ktfWindowTextDisabled, ktfEditBoxTextNormal, ktfEditBoxTextFocused, ktfEditBoxTextHot, ktfEditBoxTextDisabled, ktfEditBoxTextSelected, ktfMenuItemTextNormal, ktfMenuItemTextSelected, ktfMenuItemTextHot, ktfMenuItemTextDisabled, ktfToolItemTextNormal, ktfToolItemTextSelected, ktfToolItemTextHot, ktfToolItemTextDisabled, ktfHeaderSectionTextNormal, ktfHeaderSectionTextPressed, ktfHeaderSectionTextDraggedOut, ktfHeaderSectionTextDragging, ktfHeaderSectionTextHot, ktfHeaderSectionTextUnderDrag, ktfHeaderSectionTextDisabled, ktfStatusPanelTextNormal, ktfStatusPanelTextDisabled, ktfTabTextInactiveNormal, ktfTabTextInactiveHot, ktfTabTextInactiveDisabled, ktfTabTextActiveNormal, ktfTabTextActiveHot, ktfTabTextActiveDisabled, ktfListItemTextNormal, ktfListItemTextHot, ktfListItemTextSelected, ktfListItemTextFocused, ktfListItemTextDisabled, ktfPopupMenuItemTextNormal, ktfPopupMenuItemTextSelected, ktfPopupMenuItemTextHot, ktfPopupMenuItemTextDisabled, ktfCatgeoryButtonsNormal, ktfCatgeoryButtonsHot, ktfCatgeoryButtonsSelected, ktfCatgeoryButtonsCategoryNormal, ktfCatgeoryButtonsCategorySelected, ktfCategoryPanelGroupHeaderNormal, ktfCategoryPanelGroupHeaderHot, ktfComboBoxItemNormal, ktfComboBoxItemFocused, ktfComboBoxItemHot, ktfComboBoxItemDisabled, ktfComboBoxItemSelected, ktfGridItemNormal, ktfGridItemSelected, ktfGridItemFixedNormal, ktfGridItemFixedHot, ktfGridItemFixedPressed, ktfTreeItemTextNormal, ktfTreeItemTextHot, ktfTreeItemTextSelected, ktfTreeItemTextFocused, ktfTreeItemTextDisabled

## Output Structure

Per theme:
```
output/CODEX/
  theme.json         # colors, fonts, sys_colors, style object metadata
  style.qss          # generated Qt6 stylesheet
  assets/            # pre-sliced PNGs from bitmap atlas
    button_normal.png
    button_hot.png
    ...
```

## QSS Mapping

| VCL Element | QSS Selector |
|---|---|
| ktcWindow | `QWidget { background-color }` |
| ktcButton/Hot/Pressed/Focused/Disabled | `QPushButton` + pseudo-states |
| ktcEdit/EditDisabled | `QLineEdit` + `:disabled` |
| ktcComboBox | `QComboBox` |
| ktfButtonTextNormal etc. | `QPushButton { font; color }` per state |
| ktfCheckBoxTextNormal etc. | `QCheckBox { font; color }` per state |
| ktfStaticTextNormal | `QLabel { font; color }` |
| clWindowText | Fallback text color |
| Bitmap slices | `border-image: url(...)` / `image: url(...)` |

Color conversion: VSF `$00BBGGRR` -> QSS `#RRGGBB` (byte swap R and B).

## File Layout

```
tools/vsf_parser/
  __main__.py        # python -m vsf_parser entry point
  cli.py             # argument parsing, orchestration
  vsf_reader.py      # binary format parser
  qss_generator.py   # JSON -> QSS + sliced PNGs
```

## CLI Interface

```bash
# Parse single theme
python -m vsf_parser parse path/to/CODEX.vsf -o output/CODEX/

# Parse all themes in a directory
python -m vsf_parser parse-all path/to/Style/ -o output/

# JSON only (skip QSS generation)
python -m vsf_parser parse path/to/CODEX.vsf -o output/CODEX/ --json-only
```

## Dependencies

- Python 3.12+ stdlib: `zlib`, `struct`, `json`, `argparse`, `pathlib`
- Pillow: bitmap handling and PNG export
- No other external dependencies

## Constraints

- Only VCL_STYLE 1.0 format
- Bitmap extraction depends on understanding Delphi's TBitmap stream format
- Style object tree parsing requires implementing Delphi DFM binary deserialization
- The QSS generator maps a finite set of known VCL controls to Qt equivalents
