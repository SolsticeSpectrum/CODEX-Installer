from __future__ import annotations
import struct
import zlib
from pathlib import Path


class Bitmap:
    __slots__ = ("width", "height", "pixels")

    def __init__(self, width: int, height: int, pixels: bytearray):
        self.width  = width
        self.height = height
        self.pixels = pixels  # RGBA, top-to-bottom, 4 bytes per pixel

    @staticmethod
    def from_bgra(width: int, height: int, bgra: bytes, flip_y: bool = True) -> Bitmap:
        rgba = bytearray(len(bgra))
        stride = width * 4

        for y in range(height):
            src_y = (height - 1 - y) if flip_y else y
            src_off = src_y * stride
            dst_off = y * stride

            for x in range(width):
                s = src_off + x * 4
                d = dst_off + x * 4
                
                rgba[d]     = bgra[s + 2]
                rgba[d + 1] = bgra[s + 1]
                rgba[d + 2] = bgra[s]
                rgba[d + 3] = bgra[s + 3]

        return Bitmap(width, height, rgba)

    def crop(self, left: int, top: int, right: int, bottom: int) -> Bitmap:
        w = right - left
        h = bottom - top
        out = bytearray(w * h * 4)
        stride = self.width * 4

        for y in range(h):
            src_off = (top + y) * stride + left * 4
            dst_off = y * w * 4
            out[dst_off : dst_off + w * 4] = self.pixels[src_off : src_off + w * 4]

        return Bitmap(w, h, out)

    def clean_premultiplied(self) -> None:
        px = self.pixels
        for i in range(0, len(px), 4):
            if px[i + 3] == 0 and (px[i] | px[i + 1] | px[i + 2]) != 0:
                px[i] = px[i + 1] = px[i + 2] = 0

    def pixel(self, x: int, y: int) -> tuple[int, int, int, int]:
        off = (y * self.width + x) * 4
        return (self.pixels[off], self.pixels[off + 1], self.pixels[off + 2], self.pixels[off + 3])

    def save_png(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)

        raw = bytearray()
        stride = self.width * 4
        for y in range(self.height):
            raw.append(0)  # filter: none
            off = y * stride
            raw.extend(self.pixels[off : off + stride])

        def chunk(tag: bytes, data: bytes) -> bytes:
            crc = zlib.crc32(tag + data) & 0xFFFFFFFF
            return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)

        ihdr = struct.pack(">IIBBBBB", self.width, self.height, 8, 6, 0, 0, 0)
        idat = zlib.compress(bytes(raw))

        with open(path, "wb") as f:
            f.write(b"\x89PNG\r\n\x1a\n")
            f.write(chunk(b"IHDR", ihdr))
            f.write(chunk(b"IDAT", idat))
            f.write(chunk(b"IEND", b""))

    @staticmethod
    def load_png(path: Path) -> Bitmap:
        import struct as st

        with open(path, "rb") as f:
            sig = f.read(8)
            if sig != b"\x89PNG\r\n\x1a\n":
                raise ValueError(f"not a PNG: {path}")

            width = height = 0
            idat_chunks = []

            while True:
                hdr = f.read(8)
                if len(hdr) < 8:
                    break
                length, tag = st.unpack(">I4s", hdr)
                data = f.read(length)
                f.read(4)  # crc

                if tag == b"IHDR":
                    width, height = st.unpack(">II", data[:8])
                elif tag == b"IDAT":
                    idat_chunks.append(data)
                elif tag == b"IEND":
                    break

        raw = zlib.decompress(b"".join(idat_chunks))
        stride = width * 4
        pixels = bytearray(width * height * 4)

        for y in range(height):
            src_off = y * (stride + 1) + 1
            dst_off = y * stride
            pixels[dst_off : dst_off + stride] = raw[src_off : src_off + stride]

        return Bitmap(width, height, pixels)
