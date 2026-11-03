"""Minimal MPQ (format version 1) writer and reader for client patch archives.

Files are stored the way Blizzard stores them in its own archives: split into 16 KB sectors behind a sector
offset table, each sector zlib-compressed when that makes it smaller. Only the hash and block tables are
encrypted, as the format requires.
"""
import struct
import zlib
from typing import Dict, List

HEADER_SIZE = 32
SECTOR_SHIFT = 5  # 512 << 5 = 16 KB sectors, as in Blizzard's update archives
SECTOR_SIZE = 512 << SECTOR_SHIFT
# The 5.4.8 client fails on uncompressed single-unit files ("Reached the end of the file") and on large
# uncompressed sectored ones (SFileReadFile fails on a 16 MB table), so every file is compressed and sectored.
FILE_EXISTS = 0x80000000
FILE_COMPRESS = 0x00000200
COMPRESSION_ZLIB = 0x02
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


def build(files: Dict[str, bytes], patch_prefix: str = '') -> bytes:
    """Builds an archive holding `files` (archive path -> bytes) plus a (listfile).

    An update archive also needs `patch_prefix`, the chain it patches ("base" or a locale such as "enUS"), stored
    as (patch_metadata) the way Blizzard's own update archives carry it; without it the client attaches the
    archive to the wrong chain and its files never take effect.
    """
    entries = dict(files)
    if patch_prefix:
        entries['(patch_metadata)'] = patch_prefix.encode('ascii')
    entries['(listfile)'] = ''.join(name + '\r\n' for name in sorted(files)).encode('ascii')
    names = sorted(entries)

    body = bytearray()
    blocks = []
    for name in names:
        data = entries[name]
        packed = _pack_sectors(data)
        blocks.append((HEADER_SIZE + len(body), len(packed), len(data), FILE_EXISTS | FILE_COMPRESS))
        body += packed

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


def _pack_sectors(data: bytes) -> bytes:
    """A sector offset table, then each sector compressed (type byte, zlib) or as is when that is no smaller."""
    sectors = []
    for start in range(0, len(data), SECTOR_SIZE):
        chunk = data[start:start + SECTOR_SIZE]
        compressed = bytes([COMPRESSION_ZLIB]) + zlib.compress(chunk, 9)
        sectors.append(compressed if len(compressed) < len(chunk) else chunk)
    offsets = [4 * (len(sectors) + 1)]
    for sector in sectors:
        offsets.append(offsets[-1] + len(sector))
    return struct.pack('<%dI' % len(offsets), *offsets) + b''.join(sectors)


def _unpack_sectors(packed: bytes, size: int) -> bytes:
    count = (size + SECTOR_SIZE - 1) // SECTOR_SIZE
    offsets = struct.unpack_from('<%dI' % (count + 1), packed)
    out = bytearray()
    for i in range(count):
        chunk = packed[offsets[i]:offsets[i + 1]]
        want = min(SECTOR_SIZE, size - i * SECTOR_SIZE)
        out += zlib.decompress(chunk[1:]) if len(chunk) < want else chunk
    return bytes(out)


def read(archive: bytes, name: str) -> bytes:
    """Reads one file back; used by the tests and `build.py check`."""
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
            return _unpack_sectors(archive[pos:pos + packed], size)
        slot = (slot + 1) % hash_count
    raise KeyError(name)


def listfile(archive: bytes) -> List[str]:
    return [line for line in read(archive, '(listfile)').decode('ascii').split('\r\n') if line]
