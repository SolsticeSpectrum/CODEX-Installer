from __future__ import annotations
from pathlib import Path
from typing import Any

from .utils import theme
from .utils import png


def _crop(atlas: png.Bitmap, rect: tuple, path: Path) -> bool:
    l, t, r, b = rect
    if r <= l or b <= t:
        return False

    img = atlas.crop(l, t, r, b)
    img.clean_premultiplied()
    img.save_png(path)

    return True


# per-state bitmap prefixes
BUTTON_STATES = [
    ("Bitmap",         "normal"),
    ("BitmapHot",      "hot"),
    ("BitmapPressed",  "pressed"),
    ("BitmapDisabled", "disabled"),
    ("BitmapFocused",  "focused"),
]

CHECK_STATES = [
    ("Bitmap",         "normal"),
    ("BitmapHot",      "hot"),
    ("BitmapDisabled", "disabled"),
]

WND_BTN_STATES = [
    ("Bitmap",       "normal"),
    ("BitmapHot",    "hot"),
    ("BitmapPressed","pressed"),
    ("ActiveBitmap", "active"),
]


def slice_all(data: dict[str, Any], out: Path, atlas: png.Bitmap | None) -> None:
    if not atlas:
        return

    out.mkdir(parents=True, exist_ok=True)
    atlas.save_png(out / "style.png")

    objects = data.get("objects", [])

    # buttons
    btn = theme.find_object(objects, "Button")
    if btn:
        face = theme.find_child(btn, "Face")
        if face:
            for prefix, suffix in BUTTON_STATES:
                r = theme.bitmap_rect(face, prefix)
                if r:
                    _crop(atlas, r, out / f"button_{suffix}.png")

    # checkboxes
    chk = theme.find_object(objects, "CheckBox")
    if chk:
        for state_name in ["Unchecked", "Checked", "Mixed"]:
            child = theme.find_child(chk, state_name)
            if child:
                for prefix, suffix in CHECK_STATES:
                    r = theme.bitmap_rect(child, prefix)
                    if r:
                        _crop(atlas, r, out / f"checkbox_{state_name.lower()}_{suffix}.png")

    # edit frame
    edit = theme.find_object(objects, "Edit")
    if edit:
        bmp = theme.find_child(edit, "Frame", "bitmap")
        if bmp:
            r = theme.bitmap_rect(bmp)
            if r:
                _crop(atlas, r, out / "edit_frame.png")

    # combobox
    combo = theme.find_object(objects, "ComboBox")
    if combo:
        frame = theme.find_child(combo, "Frame", "bitmap")
        if frame:
            r = theme.bitmap_rect(frame)
            if r:
                _crop(atlas, r, out / "combobox_frame.png")

        btn = theme.find_child(combo, "Button")
        if btn:
            for prefix, suffix in BUTTON_STATES[:4]:
                r = theme.bitmap_rect(btn, prefix)
                if r:
                    _crop(atlas, r, out / f"combobox_button_{suffix}.png")

        arrow = theme.find_child(combo, "Button", "Arrow")
        if arrow:
            for prefix, suffix in CHECK_STATES:
                r = theme.bitmap_rect(arrow, prefix)
                if r:
                    _crop(atlas, r, out / f"combobox_arrow_{suffix}.png")

    # progressbar
    prog = theme.find_object(objects, "ProgressBar")
    if prog:
        frame = theme.find_child(prog, "Frame")
        if frame:
            r = theme.bitmap_rect(frame)
            if r:
                _crop(atlas, r, out / "progressbar_frame.png")

        bar = theme.find_child(prog, "BarHorz")
        if bar:
            r = theme.bitmap_rect(bar)
            if r:
                _crop(atlas, r, out / "progressbar_bar.png")

    # window frame
    form = theme.find_object(objects, "Form")
    if form:
        image = theme.find_child(form, "Image")
        if image:
            # title bar
            title = theme.find_child(image, "Title")
            if title:
                for prefix, suffix in [("Bitmap", "inactive"), ("ActiveBitmap", "active")]:
                    r = theme.bitmap_rect(title, prefix)
                    if r:
                        _crop(atlas, r, out / f"titlebar_{suffix}.png")

            # borders
            for name in ["LeftBorder", "RightBorder", "BottomBorder"]:
                border = theme.find_child(image, name)
                if border:
                    for prefix, suffix in [("Bitmap", "inactive"), ("ActiveBitmap", "active")]:
                        r = theme.bitmap_rect(border, prefix)
                        if r:
                            _crop(atlas, r, out / f"border_{name.lower()}_{suffix}.png")

            # client bg
            client = theme.find_child(image, "Client")
            if client:
                r = theme.bitmap_rect(client)
                if r:
                    _crop(atlas, r, out / "form_client.png")

            # window buttons
            caption = theme.find_child(image, "Title", "Caption")
            if caption:
                sys_btns = theme.find_child(caption, "sysButtons")
                if sys_btns:
                    for btn_name in ["btnClose", "btnMin", "btnMax", "btnRes", "btnHelp"]:
                        btn_obj = theme.find_child(sys_btns, btn_name)
                        if btn_obj:
                            for prefix, suffix in WND_BTN_STATES:
                                r = theme.bitmap_rect(btn_obj, prefix)
                                if r:
                                    _crop(atlas, r, out / f"wnd_{btn_name}_{suffix}.png")

    # scrollbar
    scroll = theme.find_object(objects, "ScrollBar")
    if scroll:
        for child_name, file_prefix in [
            ("VertFrame",  "scrollbar_vert_frame"),
            ("HorzFrame",  "scrollbar_horz_frame"),
            ("VertSlider", "scrollbar_vert_slider"),
            ("HorzSlider", "scrollbar_horz_slider"),
            ("TopButton",  "scrollbar_top_btn"),
            ("BottomButton", "scrollbar_bottom_btn"),
        ]:
            child = theme.find_child(scroll, child_name)
            if child:
                for prefix, suffix in BUTTON_STATES[:4]:
                    r = theme.bitmap_rect(child, prefix)
                    if r:
                        _crop(atlas, r, out / f"{file_prefix}_{suffix}.png")

                arrow = theme.find_child(child, "Arrow")
                if arrow:
                    for prefix, suffix in BUTTON_STATES[:4]:
                        r = theme.bitmap_rect(arrow, prefix)
                        if r:
                            _crop(atlas, r, out / f"{file_prefix}_arrow_{suffix}.png")
