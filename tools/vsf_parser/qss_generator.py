"""Generate Qt6 QSS stylesheets from parsed VSF theme data.

Replicates VCL's TSeBitmapObject.Draw 9-patch rendering by:
1. Extracting per-control-per-state bitmap crops from the atlas
2. Generating QSS border-image rules with correct slice margins
3. Mapping VCL color/font tables to Qt widget selectors
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from PIL import Image


def _find_object(objects: list[dict], name: str) -> dict | None:
    """Find a top-level style object by name."""
    for obj in objects:
        if obj.get("_name") == name:
            return obj
    return None


def _find_child(obj: dict, *names: str) -> dict | None:
    """Find a nested child by walking a path of names."""
    current = obj
    for name in names:
        found = None
        for child in current.get("_children", []):
            if child.get("_name") == name:
                found = child
                break
        if found is None:
            return None
        current = found
    return current


def _get_bitmap_rect(obj: dict, prefix: str = "Bitmap") -> tuple[int, int, int, int] | None:
    """Extract a bitmap rect (left, top, right, bottom) from an object's properties."""
    keys = [f"{prefix}.Left", f"{prefix}.Top", f"{prefix}.Right", f"{prefix}.Bottom"]
    if all(k in obj for k in keys):
        l, t, r, b = obj[keys[0]], obj[keys[1]], obj[keys[2]], obj[keys[3]]
        if r > l and b > t:
            return (l, t, r, b)
    return None


def _get_margins(obj: dict) -> tuple[int, int, int, int]:
    """Get 9-patch margins (left, top, right, bottom)."""
    return (
        obj.get("MarginLeft", 0),
        obj.get("MarginTop", 0),
        obj.get("MarginRight", 0),
        obj.get("MarginBottom", 0),
    )


def _crop_save(atlas: Image.Image, rect: tuple, path: Path) -> bool:
    """Crop a region from the atlas and save as PNG."""
    try:
        crop = atlas.crop(rect)
        if crop.width > 0 and crop.height > 0:
            crop.save(path)
            return True
    except Exception:
        pass
    return False


def _font_css(font: dict[str, Any]) -> str:
    """Convert a parsed font dict to CSS properties."""
    parts = []
    parts.append(f'font-family: "{font.get("name", "Tahoma")}";')
    parts.append(f"font-size: {font.get('size', 8)}pt;")
    parts.append(f"color: {font.get('color', '#ffffff')};")
    if font.get("style") == "bold":
        parts.append("font-weight: bold;")
    return " ".join(parts)


def _border_image_css(image_path: str, margins: tuple[int, int, int, int]) -> str:
    """Generate border-image CSS for 9-patch rendering."""
    top, right, bottom, left = margins[1], margins[2], margins[3], margins[0]
    return (
        f"border-image: url({image_path}) {top} {right} {bottom} {left} stretch stretch;\n"
        f"    border-width: {top}px {right}px {bottom}px {left}px;"
    )


def generate_qss(
    theme_data: dict[str, Any],
    assets_dir: Path | None,
    atlas: Image.Image | None = None,
) -> str:
    """Generate a Qt6 QSS stylesheet from parsed VSF theme data.

    If atlas is provided and assets_dir is set, crops per-control bitmaps
    from the atlas and generates border-image rules.
    """
    sections: list[str] = []
    theme_name = theme_data.get("name", "Unknown")
    sections.append(f"/* Auto-generated from VCL Style: {theme_name} */\n")

    colors = theme_data.get("colors", {})
    sys_colors = theme_data.get("sys_colors", {})
    fonts = theme_data.get("fonts", {})
    objects = theme_data.get("objects", [])

    def clr(key: str, fallback: str = "#000000") -> str:
        return colors.get(key, sys_colors.get(key, fallback))

    def fnt(key: str) -> dict | None:
        return fonts.get(key)

    # Determine relative asset path for QSS url() references
    asset_prefix = "assets" if assets_dir else ""

    # === Crop control bitmaps from atlas ===
    if atlas and assets_dir:
        assets_dir.mkdir(parents=True, exist_ok=True)
        atlas.save(assets_dir / "style.png")

        # Button states
        btn = _find_object(objects, "Button")
        if btn:
            face = _find_child(btn, "Face")
            if face:
                for prefix, suffix in [
                    ("Bitmap", "normal"), ("BitmapHot", "hot"),
                    ("BitmapPressed", "pressed"), ("BitmapDisabled", "disabled"),
                    ("BitmapFocused", "focused"),
                ]:
                    rect = _get_bitmap_rect(face, prefix)
                    if rect:
                        _crop_save(atlas, rect, assets_dir / f"button_{suffix}.png")

        # Checkbox states
        chk = _find_object(objects, "CheckBox")
        if chk:
            for check_state in ["Unchecked", "Checked", "Mixed"]:
                child = _find_child(chk, check_state)
                if child:
                    for prefix, suffix in [
                        ("Bitmap", "normal"), ("BitmapHot", "hot"),
                        ("BitmapDisabled", "disabled"),
                    ]:
                        rect = _get_bitmap_rect(child, prefix)
                        if rect:
                            _crop_save(atlas, rect, assets_dir / f"checkbox_{check_state.lower()}_{suffix}.png")

        # Edit frame
        edit = _find_object(objects, "Edit")
        if edit:
            frame_bmp = _find_child(edit, "Frame", "bitmap")
            if frame_bmp:
                rect = _get_bitmap_rect(frame_bmp)
                if rect:
                    _crop_save(atlas, rect, assets_dir / "edit_frame.png")

        # ComboBox frame + arrow
        combo = _find_object(objects, "ComboBox")
        if combo:
            frame_bmp = _find_child(combo, "Frame", "bitmap")
            if frame_bmp:
                rect = _get_bitmap_rect(frame_bmp)
                if rect:
                    _crop_save(atlas, rect, assets_dir / "combobox_frame.png")
            # ComboBox dropdown button face (like a mini button)
            combo_btn = _find_child(combo, "Button")
            if combo_btn:
                for prefix, suffix in [
                    ("Bitmap", "normal"), ("BitmapHot", "hot"),
                    ("BitmapPressed", "pressed"), ("BitmapDisabled", "disabled"),
                ]:
                    rect = _get_bitmap_rect(combo_btn, prefix)
                    if rect:
                        _crop_save(atlas, rect, assets_dir / f"combobox_button_{suffix}.png")
            # ComboBox dropdown arrow glyph
            arrow = _find_child(combo, "Button", "Arrow")
            if arrow:
                for prefix, suffix in [("Bitmap", "normal"), ("BitmapHot", "hot"), ("BitmapDisabled", "disabled")]:
                    rect = _get_bitmap_rect(arrow, prefix)
                    if rect:
                        _crop_save(atlas, rect, assets_dir / f"combobox_arrow_{suffix}.png")

        # ProgressBar
        prog = _find_object(objects, "ProgressBar")
        if prog:
            frame = _find_child(prog, "Frame")
            if frame:
                rect = _get_bitmap_rect(frame)
                if rect:
                    _crop_save(atlas, rect, assets_dir / "progressbar_frame.png")
            bar = _find_child(prog, "BarHorz")
            if bar:
                rect = _get_bitmap_rect(bar)
                if rect:
                    _crop_save(atlas, rect, assets_dir / "progressbar_bar.png")

        # Form title and window buttons
        form = _find_object(objects, "Form")
        if form:
            image = _find_child(form, "Image")
            if image:
                # Title bar
                title = _find_child(image, "Title")
                if title:
                    for prefix, suffix in [("Bitmap", "inactive"), ("ActiveBitmap", "active")]:
                        rect = _get_bitmap_rect(title, prefix)
                        if rect:
                            _crop_save(atlas, rect, assets_dir / f"titlebar_{suffix}.png")

                # Window borders
                for border_name in ["LeftBorder", "RightBorder", "BottomBorder"]:
                    border = _find_child(image, border_name)
                    if border:
                        for prefix, suffix in [("Bitmap", "inactive"), ("ActiveBitmap", "active")]:
                            rect = _get_bitmap_rect(border, prefix)
                            if rect:
                                _crop_save(atlas, rect, assets_dir / f"border_{border_name.lower()}_{suffix}.png")

                # Client area
                client = _find_child(image, "Client")
                if client:
                    rect = _get_bitmap_rect(client)
                    if rect:
                        _crop_save(atlas, rect, assets_dir / "form_client.png")

                # Window buttons
                caption = _find_child(image, "Title", "Caption")
                if caption:
                    sys_btns = _find_child(caption, "sysButtons")
                    if sys_btns:
                        for btn_name in ["btnClose", "btnMin", "btnMax", "btnRes", "btnHelp"]:
                            btn_obj = _find_child(sys_btns, btn_name)
                            if btn_obj:
                                for prefix, suffix in [
                                    ("Bitmap", "normal"), ("BitmapHot", "hot"),
                                    ("BitmapPressed", "pressed"), ("ActiveBitmap", "active"),
                                ]:
                                    rect = _get_bitmap_rect(btn_obj, prefix)
                                    if rect:
                                        _crop_save(atlas, rect, assets_dir / f"wnd_{btn_name}_{suffix}.png")

        # ScrollBar
        scroll = _find_object(objects, "ScrollBar")
        if scroll:
            for child_name, file_prefix in [
                ("VertFrame", "scrollbar_vert_frame"),
                ("HorzFrame", "scrollbar_horz_frame"),
                ("VertSlider", "scrollbar_vert_slider"),
                ("HorzSlider", "scrollbar_horz_slider"),
            ]:
                child = _find_child(scroll, child_name)
                if child:
                    for prefix, suffix in [
                        ("Bitmap", "normal"), ("BitmapHot", "hot"),
                        ("BitmapPressed", "pressed"), ("BitmapDisabled", "disabled"),
                    ]:
                        rect = _get_bitmap_rect(child, prefix)
                        if rect:
                            _crop_save(atlas, rect, assets_dir / f"{file_prefix}_{suffix}.png")

    # === Generate QSS ===

    # Global
    window_bg = clr("ktcWindow")
    text_color = clr("clWindowText", "#e1e0e6")
    border_color = clr("ktcBorder", "#1a1b1c")

    sections.append(f"""* {{
    outline: none;
}}

QWidget {{
    background-color: {window_bg};
    color: {text_color};
    border: none;
    font-family: "Arial";
    font-size: 9pt;
}}""")

    # --- Buttons ---
    btn_obj = _find_object(objects, "Button")
    btn_face = _find_child(btn_obj, "Face") if btn_obj else None
    btn_margins = _get_margins(btn_face) if btn_face else (7, 7, 7, 7)
    btn_font = fnt("ktfButtonTextNormal")

    if assets_dir and (assets_dir / "button_normal.png").exists():
        bi = _border_image_css(f"{asset_prefix}/button_normal.png", btn_margins)
        bi_hot = _border_image_css(f"{asset_prefix}/button_hot.png", btn_margins)
        bi_pressed = _border_image_css(f"{asset_prefix}/button_pressed.png", btn_margins)
        bi_disabled = _border_image_css(f"{asset_prefix}/button_disabled.png", btn_margins)
        bi_focused = _border_image_css(f"{asset_prefix}/button_focused.png", btn_margins)

        sections.append(f"""QPushButton {{
    {bi}
    {_font_css(btn_font) if btn_font else ''}
    padding: 2px 8px;
    min-height: 18px;
}}

QPushButton:hover {{
    {bi_hot}
    {_font_css(fnt('ktfButtonTextHot')) if fnt('ktfButtonTextHot') else ''}
}}

QPushButton:pressed {{
    {bi_pressed}
    {_font_css(fnt('ktfButtonTextPressed')) if fnt('ktfButtonTextPressed') else ''}
}}

QPushButton:disabled {{
    {bi_disabled}
    {_font_css(fnt('ktfButtonTextDisabled')) if fnt('ktfButtonTextDisabled') else ''}
}}

QPushButton:focus {{
    {bi_focused}
}}""")
    else:
        # Fallback flat colors
        sections.append(f"""QPushButton {{
    background-color: {clr('ktcButton', '#353739')};
    border: 1px solid {border_color};
    {_font_css(btn_font) if btn_font else ''}
    padding: 2px 8px;
}}""")

    # --- QLineEdit ---
    edit_obj = _find_object(objects, "Edit")
    edit_frame = _find_child(edit_obj, "Frame", "bitmap") if edit_obj else None
    edit_margins = _get_margins(edit_frame) if edit_frame else (4, 4, 4, 4)
    edit_font = fnt("ktfEditBoxTextNormal")

    if assets_dir and (assets_dir / "edit_frame.png").exists():
        bi_edit = _border_image_css(f"{asset_prefix}/edit_frame.png", edit_margins)
        sections.append(f"""QLineEdit {{
    {bi_edit}
    {_font_css(edit_font) if edit_font else ''}
    padding: 1px 2px;
}}

QLineEdit:disabled {{
    color: {clr('clGrayText', '#696969')};
}}""")
    else:
        sections.append(f"""QLineEdit {{
    background-color: {clr('ktcEdit', '#0c0c0c')};
    border: 1px solid {border_color};
    {_font_css(edit_font) if edit_font else ''}
    padding: 1px 2px;
}}""")

    # --- QComboBox ---
    combo_obj = _find_object(objects, "ComboBox")
    combo_frame = _find_child(combo_obj, "Frame", "bitmap") if combo_obj else None
    combo_margins = _get_margins(combo_frame) if combo_frame else (4, 4, 4, 4)

    if assets_dir and (assets_dir / "combobox_frame.png").exists():
        bi_combo = _border_image_css(f"{asset_prefix}/combobox_frame.png", combo_margins)
        arrow_img = f"{asset_prefix}/combobox_arrow_normal.png"
        sections.append(f"""QComboBox {{
    {bi_combo}
    {_font_css(edit_font) if edit_font else ''}
    padding: 1px 18px 1px 3px;
}}

QComboBox::drop-down {{
    border: none;
    width: 16px;
}}

QComboBox::down-arrow {{
    image: url({arrow_img});
    width: 9px;
    height: 6px;
}}

QComboBox QAbstractItemView {{
    background-color: {clr('ktcComboBox', '#1d1c1b')};
    border: 1px solid {border_color};
    selection-background-color: {clr('clHighlight', '#e86625')};
    color: {text_color};
}}""")
    else:
        sections.append(f"""QComboBox {{
    background-color: {clr('ktcComboBox', '#1d1c1b')};
    border: 1px solid {border_color};
    padding: 1px 3px;
}}""")

    # --- QCheckBox ---
    chk_font = fnt("ktfCheckBoxTextNormal")

    if assets_dir and (assets_dir / "checkbox_unchecked_normal.png").exists():
        sections.append(f"""QCheckBox {{
    background: transparent;
    spacing: 4px;
    {_font_css(chk_font) if chk_font else ''}
}}

QCheckBox::indicator {{
    width: 15px;
    height: 15px;
}}

QCheckBox::indicator:unchecked {{
    image: url({asset_prefix}/checkbox_unchecked_normal.png);
}}

QCheckBox::indicator:unchecked:hover {{
    image: url({asset_prefix}/checkbox_unchecked_hot.png);
}}

QCheckBox::indicator:unchecked:disabled {{
    image: url({asset_prefix}/checkbox_unchecked_disabled.png);
}}

QCheckBox::indicator:checked {{
    image: url({asset_prefix}/checkbox_checked_normal.png);
}}

QCheckBox::indicator:checked:hover {{
    image: url({asset_prefix}/checkbox_checked_hot.png);
}}

QCheckBox::indicator:checked:disabled {{
    image: url({asset_prefix}/checkbox_checked_disabled.png);
}}

QCheckBox:disabled {{
    color: {clr('clGrayText', '#696969')};
}}""")
    else:
        sections.append(f"""QCheckBox {{
    background: transparent;
    spacing: 4px;
    {_font_css(chk_font) if chk_font else ''}
}}

QCheckBox::indicator {{
    width: 13px;
    height: 13px;
    border: 1px solid {border_color};
    background-color: {clr('ktcEdit', '#0c0c0c')};
}}

QCheckBox::indicator:checked {{
    background-color: {clr('clHighlight', '#e86625')};
}}""")

    # --- QProgressBar ---
    prog_obj = _find_object(objects, "ProgressBar")
    prog_frame = _find_child(prog_obj, "Frame") if prog_obj else None
    prog_margins = _get_margins(prog_frame) if prog_frame else (4, 4, 4, 4)
    prog_bar = _find_child(prog_obj, "BarHorz") if prog_obj else None
    prog_bar_margins = _get_margins(prog_bar) if prog_bar else (9, 2, 9, 2)

    if assets_dir and (assets_dir / "progressbar_frame.png").exists():
        bi_prog = _border_image_css(f"{asset_prefix}/progressbar_frame.png", prog_margins)
        bi_prog_bar = _border_image_css(f"{asset_prefix}/progressbar_bar.png", prog_bar_margins)
        sections.append(f"""QProgressBar {{
    {bi_prog}
    text-align: center;
    color: {text_color};
    min-height: 15px;
}}

QProgressBar::chunk {{
    {bi_prog_bar}
}}""")
    else:
        sections.append(f"""QProgressBar {{
    background-color: {clr('ktcEdit', '#0c0c0c')};
    border: 1px solid {border_color};
    text-align: center;
}}

QProgressBar::chunk {{
    background-color: {clr('clHighlight', '#e86625')};
}}""")

    # --- QPlainTextEdit ---
    sections.append(f"""QPlainTextEdit {{
    background-color: {clr('ktcEdit', '#0c0c0c')};
    border: 1px solid {border_color};
    color: {text_color};
    font-size: 8pt;
    selection-background-color: {clr('clHighlight', '#e86625')};
}}""")

    # --- QScrollBar ---
    scroll_obj = _find_object(objects, "ScrollBar")
    vert_frame = _find_child(scroll_obj, "VertFrame") if scroll_obj else None
    vert_slider = _find_child(scroll_obj, "VertSlider") if scroll_obj else None

    if assets_dir and (assets_dir / "scrollbar_vert_frame_normal.png").exists():
        vf_margins = _get_margins(vert_frame) if vert_frame else (3, 3, 3, 3)
        vs_margins = _get_margins(vert_slider) if vert_slider else (5, 3, 5, 3)
        sections.append(f"""QScrollBar:vertical {{
    border-image: url({asset_prefix}/scrollbar_vert_frame_normal.png) {vf_margins[1]} {vf_margins[2]} {vf_margins[3]} {vf_margins[0]} stretch stretch;
    border-width: {vf_margins[1]}px {vf_margins[2]}px {vf_margins[3]}px {vf_margins[0]}px;
    width: 16px;
}}

QScrollBar::handle:vertical {{
    border-image: url({asset_prefix}/scrollbar_vert_slider_normal.png) {vs_margins[1]} {vs_margins[2]} {vs_margins[3]} {vs_margins[0]} stretch stretch;
    border-width: {vs_margins[1]}px {vs_margins[2]}px {vs_margins[3]}px {vs_margins[0]}px;
    min-height: 20px;
}}

QScrollBar::handle:vertical:hover {{
    border-image: url({asset_prefix}/scrollbar_vert_slider_hot.png) {vs_margins[1]} {vs_margins[2]} {vs_margins[3]} {vs_margins[0]} stretch stretch;
}}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
    height: 0px;
}}

QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {{
    background: none;
}}""")
    else:
        sections.append(f"""QScrollBar:vertical {{
    background-color: {clr('clScrollBar', '#1d1c1b')};
    width: 14px;
    border: none;
}}

QScrollBar::handle:vertical {{
    background-color: {clr('ktcButton', '#353739')};
    min-height: 20px;
    margin: 1px;
}}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
    height: 0px;
}}""")

    # --- QLabel ---
    label_font = fnt("ktfStaticTextNormal")
    sections.append(f"""QLabel {{
    background: transparent;
    border: none;
    {_font_css(label_font) if label_font else ''}
}}""")

    # --- QGroupBox ---
    sections.append(f"""QGroupBox {{
    background-color: {clr('ktcPanel', '#000000')};
    border: 1px solid {border_color};
    margin-top: 8px;
    padding-top: 8px;
}}""")

    # --- QFrame (bevel boxes) ---
    sections.append(f"""QFrame[frameShape="6"] {{
    border: 1px solid {border_color};
    background: transparent;
}}""")

    # --- QMessageBox ---
    sections.append(f"""QMessageBox {{
    background-color: {window_bg};
}}

QMessageBox QPushButton {{
    min-width: 70px;
}}""")

    # --- Horizontal scrollbar (hide) ---
    sections.append(f"""QScrollBar:horizontal {{
    height: 0px;
}}""")

    return "\n\n".join(sections) + "\n"
