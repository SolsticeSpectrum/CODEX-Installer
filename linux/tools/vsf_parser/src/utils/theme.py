from __future__ import annotations
import json
import sys
from pathlib import Path


def load(path: str | Path) -> dict:
    p = Path(path)
    f = p / "theme.json" if p.is_dir() else p
    if not f.exists():
        print(f"error: {f} not found", file=sys.stderr)
        sys.exit(1)

    with open(f) as fh:
        return json.load(fh)


def find_object(objects: list, path: str) -> dict | None:
    parts = path.split("/")
    for obj in objects:
        if obj.get("_name") == parts[0]:
            if len(parts) == 1:
                return obj

            return find_object(obj.get("_children", []), "/".join(parts[1:]))

    return None


def find_child(parent: dict, *names: str) -> dict | None:
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


def bitmap_rect(obj: dict, prefix: str = "Bitmap") -> tuple[int, int, int, int] | None:
    keys = [f"{prefix}.Left", f"{prefix}.Top", f"{prefix}.Right", f"{prefix}.Bottom"]
    if not all(k in obj for k in keys):
        return None

    l, t, r, b = obj[keys[0]], obj[keys[1]], obj[keys[2]], obj[keys[3]]
    if r <= l or b <= t:
        return None

    return (l, t, r, b)


def get_color(data: dict, key: str) -> str | None:
    return data.get("colors", {}).get(key) or data.get("sys_colors", {}).get(key)


def theme_dirs(path: str | Path) -> list[Path]:
    p = Path(path)
    return sorted(d for d in p.iterdir() if d.is_dir() and (d / "theme.json").exists())
