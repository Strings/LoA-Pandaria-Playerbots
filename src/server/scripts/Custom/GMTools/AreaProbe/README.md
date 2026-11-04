# Area probe

Measures a spell's area without anyone watching the client. `.areaprobe place` stands rooted probe dummies on rings
and spokes around the GM; every spell that touches a probe is recorded with where its caster stood; `.areaprobe
report` turns the hits and misses into the spell's reach, cone angle and circle. Change a radius or a filter, reload,
cast again, read the numbers.

## Commands

Permission 91010 (`Command: areaprobe`), linked to role 197.

- `.areaprobe place [step] [rings] [spokes] [arc] [friendly]`: replaces your probes with new ones. Rings every
  `step` yards (default 2) out to `step * rings` (default 8 rings), on `spokes` (default 16) spread over `arc`
  degrees (default 360) centered on your facing. Probes are hostile so enemy areas hit them; add `friendly` for
  heals and buffs. At most `GMTools.AreaProbe.MaxProbes` probes.
- `.areaprobe report [spell]`: for each spell that touched the probes (or only the one named):
  - hits and misses, and how many spokes were hit;
  - a **cone** (direction, half-angle, reach) or, when every spoke was hit, a **circle around the caster** (reach);
  - a **free circle**: center and radius around the centroid of the hits, for areas placed away from the caster;
  - **gaps** when a spoke missed closer in than it hit (minimum range, line of sight, a target cap, or an area away
    from the caster).
- `.areaprobe reset`: forgets the hits, keeps the probes.
- `.areaprobe clear`: removes the probes.

Every edge is a range: from the farthest probe inside to the nearest one outside (`11.0-13.0 yd`). Tighten it with a
smaller step or more spokes, or an arc around the edge you care about. Distances are to probe centers; most area
checks also count the target's size, which `place` prints.

Turn and move as you like between casts: each hit records its caster's position and facing, and the report measures
from there. Periodic and channeled spells are reported under the spell that hit (for Serpent's Breath, the triggered
cone 124947, not 310103).

## How it works

- `AreaProbeRules.h`: server-independent geometry. `Layout` places the slots, `ToLocal` turns a world point into the
  caster's frame, `Analyze` fits the cone and circles. Unit tests in `tests/gmtools/area_probe_test.cpp`.
- `gmtools_areaprobe.cpp`: the command and the probe AI. Probes are temporary summons of a training dummy (31144)
  with their own AI: rooted, passive, immune to knockbacks and pulls (so spells made only of those are not
  recorded), and they take no damage. `SpellHit` records the hit.

## Config

- `GMTools.AreaProbe.Lifetime` (600 s): probes despawn on their own after this, also when the GM leaves the map.
- `GMTools.AreaProbe.MaxProbes` (400): the most one `place` stands.

## Limits

- Measures the server's targeting, not the client's visual. A visual's size is a client file; size it to the
  measured reach.
- Areas with a target cap stop after the cap and show as gaps. Areas that pick random targets or chain need many
  casts.
