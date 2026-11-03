"""Read-only access to the 5.4.8 client's own MPQ chain: compressed files and Blizzard's PTCH delta patches.

Used to fetch the current version of a client file (an interface Lua file, a table) before editing it. Never
writes into the client.
"""
import bz2
import glob
import hashlib
import os
import struct
import zlib
from typing import List, Optional, Tuple

import mpq

FILE_COMPRESS = 0x00000200
FILE_ENCRYPTED = 0x00010000
FILE_PATCH = 0x00100000
FILE_SINGLE_UNIT = 0x01000000
FILE_DELETE = 0x02000000
FILE_SECTOR_CRC = 0x04000000


class Archive:
    def __init__(self, path: str):
        self.path = path
        with open(path, 'rb') as f:
            header = f.read(32)
            magic, _, _, _, shift, hash_pos, block_pos, hash_count, block_count = struct.unpack_from(
                '<4sIIHHIIII', header)
            if magic != b'MPQ\x1a':
                raise ValueError(path + ': not an MPQ archive')
            f.seek(hash_pos)
            hash_raw = f.read(hash_count * 16)
            f.seek(block_pos)
            block_raw = f.read(block_count * 16)
        self.sector = 512 << shift
        self.hash_count = hash_count
        self.block_count = block_count
        self.hashes = mpq._crypt(hash_raw, mpq.hash_string('(hash table)', mpq.HASH_KEY), True)
        self.blocks = mpq._crypt(block_raw, mpq.hash_string('(block table)', mpq.HASH_KEY), True)

    def find(self, name: str) -> Optional[Tuple[int, int, int, int]]:
        a, b = mpq.hash_string(name, mpq.HASH_A), mpq.hash_string(name, mpq.HASH_B)
        slot = mpq.hash_string(name, mpq.HASH_OFFSET) % self.hash_count
        for _ in range(self.hash_count):
            name1, name2, _, _, block_index = struct.unpack_from('<IIHHI', self.hashes, slot * 16)
            if block_index == mpq.HASH_EMPTY:
                return None
            if (name1, name2) == (a, b) and block_index < self.block_count:
                return struct.unpack_from('<IIII', self.blocks, block_index * 16)
            slot = (slot + 1) % self.hash_count
        return None

    def read(self, name: str) -> Tuple[bytes, int]:
        """Returns (data, flags). For a patch file, data is the PTCH blob."""
        entry = self.find(name)
        if not entry:
            raise KeyError(name)
        pos, packed, size, flags = entry
        if flags & FILE_ENCRYPTED:
            raise ValueError(name + ': encrypted files are not supported')
        with open(self.path, 'rb') as f:
            f.seek(pos)
            raw = f.read(packed)
        if flags & FILE_PATCH:
            # A patch file starts with an uncompressed patch info header: length, flags, data size, MD5.
            info_length, _, data_size = struct.unpack_from('<III', raw)
            raw, size = raw[info_length:], data_size
        if flags & FILE_SINGLE_UNIT:
            return (_decompress(raw) if flags & FILE_COMPRESS and len(raw) < size else raw), flags
        if not flags & FILE_COMPRESS:
            return raw[:size], flags
        count = (size + self.sector - 1) // self.sector
        offsets = struct.unpack_from('<%dI' % (count + 1), raw)
        out = bytearray()
        for i in range(count):
            chunk = raw[offsets[i]:offsets[i + 1]]
            want = min(self.sector, size - i * self.sector)
            out += _decompress(chunk) if len(chunk) < want else chunk
        return bytes(out), flags


def _decompress(data: bytes) -> bytes:
    kind, body = data[0], data[1:]
    if kind == 0x02:
        return zlib.decompress(body)
    if kind == 0x10:
        return bz2.decompress(body)
    raise ValueError('unsupported compression %#x' % kind)


def _rle_decompress(data: bytes, size: int) -> bytes:
    out = bytearray()
    i = 0
    while i < len(data) and len(out) < size:
        byte = data[i]
        i += 1
        if byte & 0x80:
            count = (byte & 0x7F) + 1
            out += data[i:i + count]
            i += count
        else:
            out += bytes(byte + 1)
    out += bytes(size - len(out))
    return bytes(out[:size])


def apply_ptch(old: bytes, patch: bytes) -> bytes:
    """Applies a Blizzard PTCH delta (COPY or BSD0) to `old`."""
    magic, patch_size, size_before, size_after = struct.unpack_from('<4sIII', patch)
    if magic != b'PTCH':
        raise ValueError('not a PTCH patch')
    if len(old) != size_before:
        raise ValueError('patch expects %d bytes, have %d' % (size_before, len(old)))
    pos = 16
    md5_magic, md5_size = struct.unpack_from('<4sI', patch, pos)
    md5_before, md5_after = patch[pos + 8:pos + 24], patch[pos + 24:pos + 40]
    if md5_magic != b'MD5_' or hashlib.md5(old).digest() != md5_before:
        raise ValueError('patch was made for a different file version')
    pos += md5_size
    xfrm_magic, xfrm_size, kind = struct.unpack_from('<4sI4s', patch, pos)
    if xfrm_magic != b'XFRM':
        raise ValueError('missing XFRM block')
    body = patch[pos + 12:pos + xfrm_size]
    if kind == b'COPY':
        new = body
    elif kind == b'BSD0':
        # The bsdiff data is RLE-packed (after a 4-byte size) when smaller than the patch's unpacked size.
        unpacked_size = patch_size - (pos + 12)
        if len(body) < unpacked_size:
            body = _rle_decompress(body[4:], unpacked_size)
        new = _bsdiff(old, body, size_after)
    else:
        raise ValueError('unsupported patch type %r' % kind)
    if hashlib.md5(new).digest() != md5_after:
        raise ValueError('patched file fails its MD5 check')
    return new


def _bsdiff(old: bytes, patch: bytes, new_size: int) -> bytes:
    signature, ctrl_size, diff_size, size = struct.unpack_from('<8sQQQ', patch)
    if signature != b'BSDIFF40' or size != new_size:
        raise ValueError('bad BSDIFF40 header')
    ctrl = patch[32:32 + ctrl_size]
    diff = patch[32 + ctrl_size:32 + ctrl_size + diff_size]
    extra = patch[32 + ctrl_size + diff_size:]
    new = bytearray(new_size)
    old_pos = new_pos = ctrl_pos = diff_pos = extra_pos = 0
    while new_pos < new_size:
        add, copy, seek = (struct.unpack_from('<I', ctrl, ctrl_pos + k)[0] for k in (0, 4, 8))
        ctrl_pos += 12
        for k in range(add):
            source = old[old_pos + k] if 0 <= old_pos + k < len(old) else 0
            new[new_pos + k] = (diff[diff_pos + k] + source) & 0xFF
        diff_pos += add
        new_pos += add
        old_pos += add
        new[new_pos:new_pos + copy] = extra[extra_pos:extra_pos + copy]
        extra_pos += copy
        new_pos += copy
        # Blizzard's control words are 32-bit sign-magnitude, not bsdiff's 64-bit offsets.
        old_pos += -(seek & 0x7FFFFFFF) if seek & 0x80000000 else seek
    return bytes(new)


def archive_chain(client_dir: str, locale: str = 'enUS') -> List[str]:
    """The client's archives in load order: base archives, then updates by build number."""
    data = os.path.join(client_dir, 'Data')
    base = sorted(p for p in glob.glob(os.path.join(data, '*.MPQ')) if 'wow-update' not in os.path.basename(p))
    locale_base = sorted(p for p in glob.glob(os.path.join(data, locale, '*.MPQ'))
                         if 'wow-update' not in os.path.basename(p))
    updates = glob.glob(os.path.join(data, 'wow-update-*.MPQ')) + glob.glob(os.path.join(data, locale, 'wow-update-*.MPQ'))
    updates.sort(key=lambda p: (int(os.path.basename(p).rsplit('-', 1)[1].split('.')[0]), 'base' not in p))
    return base + locale_base + updates


def chain_of(client_dir: str, name: str, locale: str = 'enUS', skip: Tuple[str, ...] = ()) -> str:
    """The chain ("base" or the locale) whose archives hold `name`; the client only patches a file from its own chain."""
    chain = None
    locale_dir = os.path.normcase(os.path.join(client_dir, 'Data', locale))
    for path in archive_chain(client_dir, locale):
        if os.path.basename(path) in skip or not Archive(path).find(name):
            continue
        chain = locale if os.path.normcase(os.path.dirname(path)) == locale_dir else 'base'
    if chain is None:
        raise KeyError(name)
    return chain


def read_current(client_dir: str, name: str, skip: Tuple[str, ...] = ()) -> bytes:
    """The file as the client sees it: the last full copy in load order with every later delta applied."""
    data = None
    for path in archive_chain(client_dir):
        if os.path.basename(path) in skip:
            continue
        archive = Archive(path)
        entry = archive.find(name)
        if not entry:
            continue
        if entry[3] & FILE_DELETE:
            data = None
            continue
        blob, flags = archive.read(name)
        if flags & FILE_PATCH:
            if data is None:
                raise ValueError('%s: delta in %s with no base' % (name, os.path.basename(path)))
            data = apply_ptch(data, blob)
        else:
            data = blob
    if data is None:
        raise KeyError(name)
    return data
