"""Tests for the art slicer: sizes, cropping, circle masks and colour keys."""
import json
import tempfile
import unittest
from pathlib import Path

from PIL import Image

import slice as slicer


class SliceTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        sheet = Image.new("RGB", (400, 200), (255, 0, 255))
        sheet.paste((200, 30, 30), (20, 20, 180, 180))
        sheet.paste((30, 200, 30), (220, 20, 380, 180))
        sheet.save(self.dir / "sheet.png")

    def tearDown(self):
        self.tmp.cleanup()

    def test_power_of_two_only(self):
        with self.assertRaises(ValueError):
            slicer.sizes(100)
        self.assertEqual(slicer.sizes([512, 1024]), (512, 1024))

    def test_grid_slices_and_writes_tga(self):
        for job in slicer.grid_jobs(self.dir / "sheet.png", "2x1", "icon", 64):
            out = slicer.run_job(job, self.dir, self.dir / "out")
            with Image.open(out) as img:
                self.assertEqual(img.size, (64, 64))
                self.assertEqual(img.mode, "RGBA")
        with Image.open(self.dir / "out" / "icon_2.tga") as img:
            self.assertEqual(img.getpixel((32, 32))[:3], (30, 200, 30))

    def test_circle_and_key(self):
        job = {"source": "sheet.png", "box": [0, 0, 200, 200], "out": "medal", "size": 128, "circle": True,
               "key": [255, 0, 255]}
        out = slicer.run_job(job, self.dir, self.dir)
        with Image.open(out) as img:
            self.assertEqual(img.getpixel((0, 0))[3], 0)
            self.assertEqual(img.getpixel((64, 64)), (200, 30, 30, 255))

    def test_manifest(self):
        manifest = {"source_dir": str(self.dir), "output_dir": str(self.dir / "m"),
                    "jobs": [{"source": "sheet.png", "out": "wide", "size": [256, 128]}]}
        path = self.dir / "manifest.json"
        path.write_text(json.dumps(manifest), encoding="utf-8")
        job = json.loads(path.read_text(encoding="utf-8"))["jobs"][0]
        out = slicer.run_job(job, manifest["source_dir"], manifest["output_dir"])
        with Image.open(out) as img:
            self.assertEqual(img.size, (256, 128))


if __name__ == "__main__":
    unittest.main()
