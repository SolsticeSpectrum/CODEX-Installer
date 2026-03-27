from __future__ import annotations
from pathlib import Path
from typing import Any
from PIL import Image


def _obj(objects: list[dict], name: str) -> dict | None:
    for o in objects:
        if o.get("_name") == name:
            return o
    return None


def _child(parent: dict, *names: str) -> dict | None:
    cur = parent
    for n in names:
        found = None
        for c in cur.get("_children", []):
            if c.get("_name") == n:
                found = c
                break
        if not found:
            return None
        cur = found
    return cur


def _rect(obj: dict, prefix: str = "Bitmap") -> tuple[int, int, int, int] | None:
    keys = [f"{prefix}.Left", f"{prefix}.Top", f"{prefix}.Right", f"{prefix}.Bottom"]
    if not all(k in obj for k in keys):
        return None

    l, t, r, b = obj[keys[0]], obj[keys[1]], obj[keys[2]], obj[keys[3]]
    if r <= l or b <= t:
        return None

    return (l, t, r, b)


def _crop(atlas: Image.Image, rect: tuple, path: Path) -> bool:
    try:
        img = atlas.crop(rect)
        if img.width > 0 and img.height > 0:
            img.save(path)
            return True
    except Exception:
        pass

    return False


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


def slice_all(theme: dict[str, Any], out: Path, atlas: Image.Image | None) -> None:
    if not atlas:
        return

    out.mkdir(parents=True, exist_ok=True)
    atlas.save(out / "style.png")

    objects = theme.get("objects", [])

    # buttons
    btn = _obj(objects, "Button")
    if btn:
        face = _child(btn, "Face")
        if face:
            for prefix, suffix in BUTTON_STATES:
                r = _rect(face, prefix)
                if r:
                    _crop(atlas, r, out / f"button_{suffix}.png")

    # checkboxes
    chk = _obj(objects, "CheckBox")
    if chk:
        for state_name in ["Unchecked", "Checked", "Mixed"]:
            child = _child(chk, state_name)
            if child:
                for prefix, suffix in CHECK_STATES:
                    r = _rect(child, prefix)
                    if r:
                        _crop(atlas, r, out / f"checkbox_{state_name.lower()}_{suffix}.png")

    # edit frame
    edit = _obj(objects, "Edit")
    if edit:
        bmp = _child(edit, "Frame", "bitmap")
        if bmp:
            r = _rect(bmp)
            if r:
                _crop(atlas, r, out / "edit_frame.png")

    # combobox
    combo = _obj(objects, "ComboBox")
    if combo:
        frame = _child(combo, "Frame", "bitmap")
        if frame:
            r = _rect(frame)
            if r:
                _crop(atlas, r, out / "combobox_frame.png")

        btn = _child(combo, "Button")
        if btn:
            for prefix, suffix in BUTTON_STATES[:4]:
                r = _rect(btn, prefix)
                if r:
                    _crop(atlas, r, out / f"combobox_button_{suffix}.png")

        arrow = _child(combo, "Button", "Arrow")
        if arrow:
            for prefix, suffix in CHECK_STATES:
                r = _rect(arrow, prefix)
                if r:
                    _crop(atlas, r, out / f"combobox_arrow_{suffix}.png")

    # progressbar
    prog = _obj(objects, "ProgressBar")
    if prog:
        frame = _child(prog, "Frame")
        if frame:
            r = _rect(frame)
            if r:
                _crop(atlas, r, out / "progressbar_frame.png")

        bar = _child(prog, "BarHorz")
        if bar:
            r = _rect(bar)
            if r:
                _crop(atlas, r, out / "progressbar_bar.png")

    # window frame
    form = _obj(objects, "Form")
    if form:
        image = _child(form, "Image")
        if image:
            # title bar
            title = _child(image, "Title")
            if title:
                for prefix, suffix in [("Bitmap", "inactive"), ("ActiveBitmap", "active")]:
                    r = _rect(title, prefix)
                    if r:
                        _crop(atlas, r, out / f"titlebar_{suffix}.png")

            # borders
            for name in ["LeftBorder", "RightBorder", "BottomBorder"]:
                border = _child(image, name)
                if border:
                    for prefix, suffix in [("Bitmap", "inactive"), ("ActiveBitmap", "active")]:
                        r = _rect(border, prefix)
                        if r:
                            _crop(atlas, r, out / f"border_{name.lower()}_{suffix}.png")

            # client bg
            client = _child(image, "Client")
            if client:
                r = _rect(client)
                if r:
                    _crop(atlas, r, out / "form_client.png")

            # window buttons
            caption = _child(image, "Title", "Caption")
            if caption:
                sys_btns = _child(caption, "sysButtons")
                if sys_btns:
                    for btn_name in ["btnClose", "btnMin", "btnMax", "btnRes", "btnHelp"]:
                        btn_obj = _child(sys_btns, btn_name)
                        if btn_obj:
                            for prefix, suffix in WND_BTN_STATES:
                                r = _rect(btn_obj, prefix)
                                if r:
                                    _crop(atlas, r, out / f"wnd_{btn_name}_{suffix}.png")

    # scrollbar
    scroll = _obj(objects, "ScrollBar")
    if scroll:
        for child_name, file_prefix in [
            ("VertFrame",  "scrollbar_vert_frame"),
            ("HorzFrame",  "scrollbar_horz_frame"),
            ("VertSlider", "scrollbar_vert_slider"),
            ("HorzSlider", "scrollbar_horz_slider"),
        ]:
            child = _child(scroll, child_name)
            if child:
                for prefix, suffix in BUTTON_STATES[:4]:
                    r = _rect(child, prefix)
                    if r:
                        _crop(atlas, r, out / f"{file_prefix}_{suffix}.png")
