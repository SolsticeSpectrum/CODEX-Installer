from __future__ import annotations
import struct
from typing import Any



class DfmReader:
    def __init__(self, data: bytes):
        self.data = data
        self.pos  = 0


    def short_string(self) -> str:
        length    = self.data[self.pos]
        self.pos += 1
        value     = self.data[self.pos : self.pos + length].decode("ascii", errors="replace")
        self.pos += length

        return value


    def value(self, typ: int) -> Any:
        if typ == 0x01:  # list
            items = []
            while self.pos < len(self.data):
                t = self.data[self.pos]
                if t == 0x00:
                    self.pos += 1
                    break
                
                self.pos += 1
                items.append(self.value(t))
                
            return items

        if typ == 0x02:  # int8
            v = struct.unpack_from("<b", self.data, self.pos)[0]
            self.pos += 1
            
            return v

        if typ == 0x03:  # int16
            v = struct.unpack_from("<h", self.data, self.pos)[0]
            self.pos += 2
            
            return v

        if typ == 0x04:  # int32
            v = struct.unpack_from("<i", self.data, self.pos)[0]
            self.pos += 4
            
            return v

        if typ == 0x05:  # extended float (10 bytes)
            try:
                v = struct.unpack_from("<d", self.data, self.pos)[0]
            except struct.error:
                v = 0.0
            self.pos += 10
            
            return v

        if typ in (0x06, 0x07):  # short string / ident
            return self.short_string()

        if typ == 0x08:  # false
            return False

        if typ == 0x09:  # true
            return True

        if typ == 0x0A:  # binary blob
            length    = struct.unpack_from("<I", self.data, self.pos)[0]
            self.pos += 4
            
            blob      = self.data[self.pos : self.pos + length]
            self.pos += length
            
            return blob

        if typ == 0x0B:  # set
            items = []
            while self.pos < len(self.data):
                s = self.short_string()
                if s == "":
                    break
                
                items.append(s)
                
            return items

        if typ == 0x0C:  # long string
            length    = struct.unpack_from("<I", self.data, self.pos)[0]
            self.pos += 4
            
            s         = self.data[self.pos : self.pos + length].decode("ascii", errors="replace")
            self.pos += length
            
            return s

        if typ == 0x0D:  # nil
            return None

        if typ == 0x12:  # utf8 string
            length    = struct.unpack_from("<I", self.data, self.pos)[0]
            self.pos += 4
            
            s         = self.data[self.pos : self.pos + length].decode("utf-8", errors="replace")
            self.pos += length
            
            return s

        if typ == 0x14:  # int64
            v = struct.unpack_from("<q", self.data, self.pos)[0]
            self.pos += 8
            
            return v

        raise ValueError(f"unknown DFM type 0x{typ:02x} at {self.pos}")


    def object(self) -> dict[str, Any]:
        class_name = self.short_string()
        inst_name  = self.short_string()

        obj: dict[str, Any] = {
            "_class": class_name,
            "_name":  inst_name,
        }

        # properties until 0x00
        while self.pos < len(self.data):
            if self.data[self.pos] == 0x00:
                self.pos += 1
                break

            prop      = self.short_string()
            typ       = self.data[self.pos]
            self.pos += 1

            obj[prop] = self.value(typ)

        # children until 0x00
        children = []
        while self.pos < len(self.data) and self.data[self.pos] != 0x00:
            children.append(self.object())

        if self.pos < len(self.data) and self.data[self.pos] == 0x00:
            self.pos += 1

        if children:
            obj["_children"] = children

        return obj



def parse(data: bytes) -> dict[str, Any]:
    if data[:4] != b"TPF0":
        raise ValueError(f"not a TPF0 stream (got {data[:4]!r})")

    reader = DfmReader(data)
    reader.pos = 4

    return reader.object()
