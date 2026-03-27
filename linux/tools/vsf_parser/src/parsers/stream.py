import struct


# int32 char_count + char_count * 2 bytes of UTF-16LE
def read_string(data: bytes, pos: int) -> tuple[str, int]:
    count = struct.unpack_from("<I", data, pos)[0]
    pos += 4

    nbytes = count * 2
    text = data[pos : pos + nbytes].decode("utf-16-le")

    return text, pos + nbytes
