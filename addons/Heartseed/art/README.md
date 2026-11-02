# Heartseed panel art

The panel (`Heartseed.lua`) draws built-in textures until this folder has its art. Then set `USE_ART = true` at the
top of `Heartseed.lua`: the client draws nothing for a missing file, so the switch waits until every file below
is here.

## Files

| File | Size | Shape | Used for |
|---|---|---|---|
| `background.tga` | 512x512 | square, opaque | panel background behind everything |
| `seed.tga` | 128x128 | circle | the seed in the middle of the mandala |
| `ring.tga` | 128x128 | ring, clear centre | gold frame around the seed, Celestials and major nodes |
| `glow.tga` | 256x256 | soft radial, black background | pulsing light behind the seed (drawn additive, tinted by path) |
| `halo.tga` | 256x256 | soft radial, black background | wide faint halo (additive, tinted) |
| `xuen.tga` | 128x128 | circle | Xuen medallion, also the icon for Xuen majors |
| `niuzao.tga` | 128x128 | circle | Niuzao medallion |
| `yulon.tga` | 128x128 | circle | Yu'lon medallion |
| `chiji.tga` | 128x128 | circle | Chi-Ji medallion |

Generate at 1024x1024 or larger, then slice with `tools/art-slicer` and `manifest.json` here:

```bash
python tools/art-slicer/slice.py addons/Heartseed/art/manifest.json
```

Edit `source_dir` in the manifest to where the generated images are. Additive textures (`glow`, `halo`) need a pure
black background, not transparency: black adds nothing.

## Prompts

Paste the style block first in every prompt, so the set matches.

**Style block**

> Hand-painted fantasy game UI art in the style of Mists of Pandaria's interface: warm jade, gold and lacquer-red
> palette, soft painterly brushwork, subtle ink-wash texture, gentle rim light, no text, no letters, no watermark,
> centred subject, clean edges, readable at small size.

**Medallions** (one image each, or one 2x2 sheet: then use `--grid sheet.png 2x2 medal 128` and rename)

1. `xuen`: round medallion, the face of Xuen, a majestic white tiger with blue-white crackling lightning in its fur,
   fierce but noble, framed by a thin gold rim, plain dark jade background.
2. `niuzao`: round medallion, the face of Niuzao, a massive black ox with bronze horns and calm, immovable eyes,
   warm amber light, thin gold rim, plain dark jade background.
3. `yulon`: round medallion, Yu'lon, a jade serpent dragon coiled in a circle with flowing whiskers and soft green
   glow, thin gold rim, plain dark jade background.
4. `chiji`: round medallion, Chi-Ji, a red crane with outstretched wings and golden fire-tipped feathers, hopeful
   and graceful, thin gold rim, plain dark jade background.

**Seed**

5. `seed`: a glowing seed of light the size of a fist, translucent jade shell with a golden heart pulsing inside,
   four faint coloured threads (white, black-bronze, jade, red) curling around it, round composition, dark
   background.

**Frames and light**

6. `ring`: a thin ornate gold ring with Pandaren cloud-scroll engravings and four small jade studs at the compass
   points, perfectly circular, transparent (or flat magenta) centre and background.
7. `glow`: a soft radial burst of golden light, brightest in the centre fading smoothly to pure black at the edges,
   no shapes, pure black background.
8. `halo`: a very soft, wide circular aura of light with faint mist wisps, fading to pure black, pure black
   background.

**Background**

9. `background`: a dark jade lacquered panel with faint painted mist and clouds, a subtle circular mandala
   engraving in the centre, four faint animal silhouettes at the compass points (tiger west, ox north, serpent east,
   crane south), low contrast so text stays readable, no text.
