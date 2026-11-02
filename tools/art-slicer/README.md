# Art slicer

Turns generated images (for example from an image model) into textures the 5.4.8 client loads from an addon folder:
32-bit TGA, power-of-two sides up to 1024.

## Interface

```bash
python tools/art-slicer/slice.py <manifest.json>
python tools/art-slicer/slice.py --grid sheet.png 2x2 medal 128 --out addons/Heartseed/art
```

A manifest lists `source_dir`, `output_dir` and jobs: `source`, optional `box` (crop in source pixels), `out`
(name without extension), `size` (one number or `[width, height]`), optional `circle` (round mask with a soft
edge, `feather` pixels) and `key` (`[r, g, b]` made transparent, `tolerance` default 24). Images are scaled to cover
the target and centre-cropped, never stretched. Example: `addons/Heartseed/art/manifest.json`.

Needs Pillow. Tests: `python -m unittest test_slice` in this folder.
