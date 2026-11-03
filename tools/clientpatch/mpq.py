"""Minimal MPQ (format version 1) writer and reader for client patch archives.

Files are stored uncompressed and unencrypted, which every MPQ-reading WoW client accepts. Only the hash and
block tables are encrypted, as the format requires.
"""
import struct
from typing import Dict, List

HEADER_SIZE = 32
SECTOR_SHIFT = 3  # 512 << 3 = 4096-byte sectors
FILE_EXISTS = 0x80000000
# One raw unit with no sector offset table: every MPQ reader agrees on that layout for uncompressed data.
FILE_SINGLE_UNIT = 0x01000000
HASH_EMPTY = 0xFFFFFFFF


def _crypt_table():
    table = [0] * 0x500
    seed = 0x00100001
    for index1 in range(0x100):
        index2 = index1
        for _ in range(5):
            seed = (seed * 125 + 3) % 0x2AAAAB
            high = (seed & 0xFFFF) << 16
            seed = (seed * 125 + 3) % 0x2AAAAB
            table[index2] = high | (seed & 0xFFFF)
            index2 += 0x100
    return table


CRYPT = _crypt_table()
HASH_OFFSET, HASH_A, HASH_B, HASH_KEY = 0, 1, 2, 3


def hash_string(name: str, kind: int) -> int:
    seed1, seed2 = 0x7FED7FED, 0xEEEEEEEE
    for ch in name.upper().replace('/', '\\'):
        value = ord(ch)
        seed1 = CRYPT[(kind << 8) + value] ^ ((seed1 + seed2) & 0xFFFFFFFF)
        seed2 = (value + seed1 + seed2 + (seed2 << 5) + 3) & 0xFFFFFFFF
    return seed1


def _crypt(data: bytes, key: int, decrypt: bool) -> bytes:
    words = struct.unpack('<%dI' % (len(data) // 4), data)
    seed = 0xEEEEEEEE
    out = []
    for word in words:
        seed = (seed + CRYPT[0x400 + (key & 0xFF)]) & 0xFFFFFFFF
        result = word ^ ((key + seed) & 0xFFFFFFFF)
        plain = result if decrypt else word
        key = ((((~key) << 0x15) + 0x11111111) | (key >> 0x0B)) & 0xFFFFFFFF
        seed = (plain + seed + (seed << 5) + 3) & 0xFFFFFFFF
        out.append(result)
    return struct.pack('<%dI' % len(out), *out)


def _table_size(count: int) -> int:
    size = 16
    while size < count * 2:
        size *= 2
    return size


def build(files: Dict[str, bytes]) -> bytes:
    """Builds an archive holding `files` (archive path -> bytes) plus a (listfile)."""
    entries = dict(files)
    entries['(listfile)'] = ''.join(name + '\r\n' for name in sorted(files)).encode('ascii')
    names = sorted(entries)

    body = bytearray()
    blocks = []
    for name in names:
        data = entries[name]
        blocks.append((HEADER_SIZE + len(body), len(data), len(data), FILE_EXISTS | FILE_SINGLE_UNIT))
        body += data

    size = _table_size(len(names))
    hashes = [(HASH_EMPTY, HASH_EMPTY, 0xFFFF, 0xFFFF, HASH_EMPTY)] * size
    for block_index, name in enumerate(names):
        slot = hash_string(name, HASH_OFFSET) % size
        while hashes[slot][4] != HASH_EMPTY:
            slot = (slot + 1) % size
        # Locale 0 is neutral: the file applies to every client language.
        hashes[slot] = (hash_string(name, HASH_A), hash_string(name, HASH_B), 0, 0, block_index)

    hash_raw = b''.join(struct.pack('<IIHHI', *entry) for entry in hashes)
    block_raw = b''.join(struct.pack('<IIII', *entry) for entry in blocks)
    hash_pos = HEADER_SIZE + len(body)
    block_pos = hash_pos + len(hash_raw)
    total = block_pos + len(block_raw)
    header = struct.pack('<4sIIHHIIII', b'MPQ\x1a', HEADER_SIZE, total, 0, SECTOR_SHIFT,
                         hash_pos, block_pos, size, len(blocks))
    return (header + bytes(body)
            + _crypt(hash_raw, hash_string('(hash table)', HASH_KEY), False)
            + _crypt(block_raw, hash_string('(block table)', HASH_KEY), False))


def read(archive: bytes, name: str) -> bytes:
    """Reads one uncompressed file back; used by the tests and `build.py check`."""
    magic, _, _, _, _, hash_pos, block_pos, hash_count, block_count = struct.unpack_from('<4sIIHHIIII', archive)
    if magic != b'MPQ\x1a':
        raise ValueError('not an MPQ archive')
    hash_raw = _crypt(archive[hash_pos:hash_pos + hash_count * 16], hash_string('(hash table)', HASH_KEY), True)
    block_raw = _crypt(archive[block_pos:block_pos + block_count * 16], hash_string('(block table)', HASH_KEY), True)
    slot = hash_string(name, HASH_OFFSET) % hash_count
    a, b = hash_string(name, HASH_A), hash_string(name, HASH_B)
    for _ in range(hash_count):
        name1, name2, _, _, block_index = struct.unpack_from('<IIHHI', hash_raw, slot * 16)
        if block_index == HASH_EMPTY:
            break
        if (name1, name2) == (a, b):
            pos, packed, size, _ = struct.unpack_from('<IIII', block_raw, block_index * 16)
            if packed != size:
                raise ValueError('compressed files are not supported')
            return archive[pos:pos + size]
        slot = (slot + 1) % hash_count
    raise KeyError(name)


def listfile(archive: bytes) -> List[str]:
    return [line for line in read(archive, '(listfile)').decode('ascii').split('\r\n') if line]
