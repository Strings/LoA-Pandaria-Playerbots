import hashlib
import struct
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import build  # noqa: E402
import client  # noqa: E402


def ptch(old: bytes, new: bytes, kind: bytes, body: bytes) -> bytes:
    xfrm = b'XFRM' + struct.pack('<I', 12 + len(body)) + kind + body
    md5 = b'MD5_' + struct.pack('<I', 40) + hashlib.md5(old).digest() + hashlib.md5(new).digest()
    header_size = 16 + len(md5) + 12
    # The PTCH size field counts the unpacked patch, which the RLE check compares against.
    return b'PTCH' + struct.pack('<III', header_size + len(body), len(old), len(new)) + md5 + xfrm


def bsdiff(ctrl, diff: bytes, extra: bytes, new_size: int) -> bytes:
    ctrl_raw = b''.join(struct.pack('<III', *row) for row in ctrl)
    return struct.pack('<8sQQQ', b'BSDIFF40', len(ctrl_raw), len(diff), new_size) + ctrl_raw + diff + extra


class PtchTests(unittest.TestCase):
    def test_copy(self):
        self.assertEqual(client.apply_ptch(b'old', ptch(b'old', b'brand new', b'COPY', b'brand new')), b'brand new')

    def test_bsd0_add_extra_and_backward_seek(self):
        old = b'local LEVEL = 20;'
        new = b'local LEVEL = 1;local'
        # Keep 14 bytes, append "1;" from extra, seek back 14 to the start, then copy "local" again.
        diff = bytes(14) + bytes(5)
        ctrl = [(14, 2, 0x80000000 | 14), (5, 0, 0)]
        body = bsdiff(ctrl, diff, b'1;', len(new))
        self.assertEqual(client.apply_ptch(old, ptch(old, new, b'BSD0', body)), new)

    def test_bsd0_rle_packed(self):
        old = bytes(400)
        new = bytes(400)
        raw = bsdiff([(400, 0, 0)], bytes(400), b'', 400)
        # RLE: each 0x7F run byte stands for 128 zeros; literal runs copy bytes as they are.
        packed = bytearray(struct.pack('<I', len(raw)))
        for start in range(0, len(raw), 128):
            run = raw[start:start + 128]
            packed += bytes([0x80 | (len(run) - 1)]) + run if any(run) else bytes([len(run) - 1])
        patch = ptch(old, new, b'BSD0', bytes(packed))
        # Fix the size field to the unpacked length so the decoder sees a packed body.
        patch = patch[:4] + struct.pack('<I', 68 + len(raw)) + patch[8:]
        self.assertEqual(client.apply_ptch(old, patch), new)

    def test_rejects_wrong_base(self):
        with self.assertRaises(ValueError):
            client.apply_ptch(b'abc', ptch(b'xyz', b'new', b'COPY', b'new'))

    def test_rejects_bad_result(self):
        patch = bytearray(ptch(b'old', b'new', b'COPY', b'new'))
        patch[-1:] = b'X'
        with self.assertRaises(ValueError):
            client.apply_ptch(b'old', bytes(patch))


class TextEditTests(unittest.TestCase):
    def test_replaces_one_line(self):
        out = build.patch_text(b'a\nlocal X = 20;\nb\n', 'f.lua', [{'from': 'local X = 20;', 'to': 'local X = 1;'}])
        self.assertEqual(out, b'a\nlocal X = 1;\nb\n')

    def test_refuses_missing_or_repeated(self):
        for source in (b'nothing', b'local X = 20;local X = 20;'):
            with self.assertRaises(ValueError):
                build.patch_text(source, 'f.lua', [{'from': 'local X = 20;', 'to': 'local X = 1;'}])


if __name__ == '__main__':
    unittest.main()
