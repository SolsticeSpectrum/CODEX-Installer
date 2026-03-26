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
            "clScrollBar": "#1d1c1b",
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
