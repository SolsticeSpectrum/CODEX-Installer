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
    colors = data.get("colors", {})
    if key in colors:
        return colors[key]
    sys_colors = data.get("sys_colors", {})
    if key in sys_colors:
        return sys_colors[key]
    return fallback


def _get_font(data: dict, key: str) -> dict[str, Any] | None:
    return data.get("fonts", {}).get(key)


def generate_qss(theme_data: dict[str, Any], assets_dir: Path | None) -> str:
    """Generate a Qt6 QSS stylesheet from parsed VSF theme data."""
    sections: list[str] = []
    sections.append(f"/* Auto-generated from VCL Style: {theme_data.get('name', 'Unknown')} */")
    sections.append("")

    window_color = _get_color(theme_data, "ktcWindow")
    window_text = _get_color(theme_data, "clWindowText", "#e0e0e0")
    sections.append(f"""QWidget {{
    background-color: {window_color};
    color: {window_text};
}}""")

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

    sections.append(f"""QPushButton:hover {{
    background-color: {btn_bg_hover};
    {_font_css(btn_font_hover) if btn_font_hover else ''}
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

    chk_font = _get_font(theme_data, "ktfCheckBoxTextNormal")
    sections.append(f"""QCheckBox {{
    spacing: 5px;
    {_font_css(chk_font) if chk_font else ''}
}}""")

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

    sections.append(f"""QTextEdit, QPlainTextEdit {{
    background-color: {edit_bg};
    border: 1px solid {btn_border};
    {_font_css(edit_font) if edit_font else ''}
}}""")

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

    listbox_bg = _get_color(theme_data, "ktcListBox", "#1d1c1b")
    tree_bg = _get_color(theme_data, "ktcTreeView", "#3c3c3c")
    sections.append(f"""QTreeView, QListView {{
    background-color: {listbox_bg};
    border: 1px solid {btn_border};
    alternate-background-color: {tree_bg};
}}""")

    sections.append(f"""QTreeView::item:selected, QListView::item:selected {{
    background-color: {highlight};
}}""")

    label_font = _get_font(theme_data, "ktfStaticTextNormal")
    sections.append(f"""QLabel {{
    background: transparent;
    {_font_css(label_font) if label_font else ''}
}}""")

    panel_bg = _get_color(theme_data, "ktcPanel")
    sections.append(f"""QGroupBox {{
    background-color: {panel_bg};
    border: 1px solid {btn_border};
    margin-top: 8px;
    padding-top: 8px;
}}""")

    return "\n\n".join(sections) + "\n"
