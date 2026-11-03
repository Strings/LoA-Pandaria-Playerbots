"""A WDBC table held as raw 32-bit cells plus its string block, so rows can be edited, cloned and appended."""
import struct
from typing import Dict, List


class Dbc:
    def __init__(self, raw: bytes, name: str = ''):
        magic, count, fields, record_size, string_size = struct.unpack_from('<4sIIII', raw)
        if magic != b'WDBC' or record_size != fields * 4:
            raise ValueError(name + ': only WDBC tables with 4-byte fields are supported')
        self.name = name
        self.fields = fields
        self.rows: List[List[int]] = [list(struct.unpack_from('<%dI' % fields, raw, 20 + i * record_size))
                                      for i in range(count)]
        start = 20 + count * record_size
        self.strings = bytearray(raw[start:start + string_size]) or bytearray(b'\0')
        self._offsets: Dict[bytes, int] = {}

    def string(self, offset: int) -> str:
        end = self.strings.index(b'\0', offset)
        return self.strings[offset:end].decode('utf-8')

    def add_string(self, text: str) -> int:
        if not text:
            return 0
        data = text.encode('utf-8')
        if data not in self._offsets:
            self._offsets[data] = len(self.strings)
            self.strings += data + b'\0'
        return self._offsets[data]

    def find(self, column: int, value: int) -> List[List[int]]:
        return [row for row in self.rows if row[column] == value]

    def append(self, row: List[int]):
        self.insert(len(self.rows), row)

    def insert(self, index: int, row: List[int]):
        if len(row) != self.fields:
            raise ValueError('%s: row has %d cells, table has %d' % (self.name, len(row), self.fields))
        if self.find(0, row[0]):
            raise ValueError('%s: id %d already exists' % (self.name, row[0]))
        self.rows.insert(index, list(row))

    def to_bytes(self) -> bytes:
        out = bytearray(struct.pack('<4sIIII', b'WDBC', len(self.rows), self.fields, self.fields * 4,
                                    len(self.strings)))
        for row in self.rows:
            out += struct.pack('<%dI' % self.fields, *row)
        return bytes(out + self.strings)


def as_float(cell: int) -> float:
    return struct.unpack('<f', struct.pack('<I', cell))[0]


def as_int(cell: int) -> int:
    return struct.unpack('<i', struct.pack('<I', cell))[0]
