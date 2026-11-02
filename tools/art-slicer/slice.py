"""Slice generated art into WoW-ready textures: crop, square, resize to a power of two, write 32-bit TGA.

The 5.4.8 client loads .tga and .blp from an addon folder; sides must be powers of two (up to 1024). Each job in
the manifest names a source image, an optional crop box in source pixels, the output name and size:

    {
      "source_dir": "C:/art/heartseed",
      "output_dir": "I:/UltimateMoP/.worktrees/claude/addons/Heartseed/art",
      "jobs": [
        {"source": "sheet.png", "box": [0, 0, 512, 512], "out": "xuen", "size": 128, "circle": true},
        {"source": "background.png", "out": "background", "size": [512, 1024]}
      ]
    }

"circle" fades everything outside the inscribed circle to transparent, for round medallions. A source without
alpha keeps its background unless "key" names a color to make transparent ([r, g, b], tolerance 24).

    python tools/art-slicer/slice.py manifest.json [--grid sheet.png 4x4 out_prefix 128]
"""
import argparse
import json
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

LANCZOS = getattr(Image, "Resampling", Image).LANCZOS


def power_of_two(n):
    if n < 1 or n > 1024 or n & (n - 1):
        raise ValueError("texture sides must be powers of two from 1 to 1024, got %d" % n)
    return n


def sizes(value):
    if isinstance(value, int):
        return power_of_two(value), power_of_two(value)
    return power_of_two(value[0]), power_of_two(value[1])


def key_out(img, color, tolerance=24):
    img = img.convert("RGBA")
    px = img.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = px[x, y]
            if abs(r - color[0]) <= tolerance and abs(g - color[1]) <= tolerance and abs(b - color[2]) <= tolerance:
                px[x, y] = (r, g, b, 0)
    return img


def circle_mask(img, feather=2):
    mask = Image.new("L", img.size, 0)
    inset = max(1, feather)
    ImageDraw.Draw(mask).ellipse((inset, inset, img.width - inset - 1, img.height - inset - 1), fill=255)
    if feather:
        mask = mask.filter(ImageFilter.GaussianBlur(feather))
    out = img.convert("RGBA")
    alpha = Image.composite(out.getchannel("A"), Image.new("L", img.size, 0), mask)
    out.putalpha(alpha)
    return out


def fit(img, width, height):
    """Scale to cover the target and centre-crop, so nothing is stretched."""
    scale = max(width / img.width, height / img.height)
    resized = img.resize((max(width, math.ceil(img.width * scale)), max(height, math.ceil(img.height * scale))),
                         LANCZOS)
    left = (resized.width - width) // 2
    top = (resized.height - height) // 2
    return resized.crop((left, top, left + width, top + height))


def run_job(job, source_dir, output_dir):
    img = Image.open(Path(source_dir) / job["source"]).convert("RGBA")
    if "box" in job:
        img = img.crop(tuple(job["box"]))
    if "key" in job:
        img = key_out(img, job["key"], job.get("tolerance", 24))
    width, height = sizes(job.get("size", 128))
    img = fit(img, width, height)
    if job.get("circle"):
        img = circle_mask(img, job.get("feather", 2))
    out = Path(output_dir) / (job["out"] + ".tga")
    out.parent.mkdir(parents=True, exist_ok=True)
    img.save(out, format="TGA", compression=None)
    return out


def grid_jobs(sheet, grid, prefix, size):
    cols, rows = (int(v) for v in grid.lower().split("x"))
    with Image.open(sheet) as img:
        cw, ch = img.width // cols, img.height // rows
    return [{"source": Path(sheet).name, "box": [c * cw, r * ch, (c + 1) * cw, (r + 1) * ch],
             "out": "%s_%d" % (prefix, r * cols + c + 1), "size": size}
            for r in range(rows) for c in range(cols)]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("manifest", nargs="?")
    parser.add_argument("--grid", nargs=4, metavar=("SHEET", "COLSxROWS", "PREFIX", "SIZE"),
                        help="slice an evenly spaced sheet without a manifest")
    parser.add_argument("--out", help="output directory for --grid")
    args = parser.parse_args()

    if args.grid:
        sheet, grid, prefix, size = args.grid
        jobs = grid_jobs(sheet, grid, prefix, int(size))
        source_dir, output_dir = Path(sheet).parent, args.out or "."
    elif args.manifest:
        manifest = json.loads(Path(args.manifest).read_text(encoding="utf-8"))
        jobs, source_dir, output_dir = manifest["jobs"], manifest["source_dir"], manifest["output_dir"]
    else:
        parser.error("give a manifest or --grid")

    for job in jobs:
        print(run_job(job, source_dir, output_dir))


if __name__ == "__main__":
    main()
