from __future__ import annotations
from typing import Any
from ..constants import NAMED_COLORS


# $00BBGGRR -> #RRGGBB, also handles clBlack etc
def bgr_to_rgb(color: str) -> str:
    color = color.strip()

    if color in NAMED_COLORS:
        return NAMED_COLORS[color]

    if color.startswith("$00") and len(color) == 9:
        b, g, r = color[3:5], color[5:7], color[7:9]
        return f"#{r}{g}{b}".lower()

    if color.startswith("$") and len(color) >= 7:
        padded = color[1:].zfill(8)[-6:]
        b, g, r = padded[0:2], padded[2:4], padded[4:6]
        return f"#{r}{g}{b}".lower()

    return color


# "Name,Size,Charset,R,G,B[,style]"
def parse_font(spec: str) -> dict[str, Any]:
    parts = spec.split(",")
    out: dict[str, Any] = {
        "name":    parts[0].strip() if parts else "Tahoma",
        "size":    int(parts[1].strip()) if len(parts) > 1 else 8,
        "charset": int(parts[2].strip()) if len(parts) > 2 else 1,
        "color":   "#000000",
        "style":   "",
    }

    if len(parts) >= 6:
        r, g, b = int(parts[3].strip()), int(parts[4].strip()), int(parts[5].strip())
        out["color"] = f"#{r:02x}{g:02x}{b:02x}"

    if len(parts) >= 7:
        out["style"] = parts[6].strip()

    return out
