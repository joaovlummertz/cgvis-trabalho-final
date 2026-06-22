# Interactive map door groups

`fullmap.obj` exports reset custom group names to `Brush...`. This file records
the geometry and material signatures needed to restore the interactive names.
OBJ bounds below are unscaled; the game loads the map at scale `0.02`.

## GlassDoor1

- Original export groups: `Brush1` through `Brush34`
- Renamed groups: `GlassDoor1_01` through `GlassDoor1_34`
- Materials:
  - Parts `01–06`: `GLASS_MED`
  - Parts `07–34`: `C1A1W1D`
- Combined OBJ bounds: `(-2749, -288, 765)` to `(-2741, -180, 841)`
- Motion: sideways along `+Z` by `1.7` world units

## GlassDoor2

- Original export groups: `Brush35` through `Brush68`
- Renamed groups: `GlassDoor2_01` through `GlassDoor2_34`
- Materials:
  - Parts `01–28`: `C1A1W1D`
  - Parts `29–34`: `GLASS_MED`
- Combined OBJ bounds: `(-2277, -288, 765)` to `(-2269, -180, 841)`
- Motion: sideways along `-Z` by `1.7` world units

## BlastDoor1

- Original export groups: `Brush987` through `Brush998`
- Renamed groups: `BlastDoor1_01` through `BlastDoor1_12`
- Materials:
  - Parts `01–03`, `06–08`, `11–12`: `C1A1DOOREDGE`
  - Parts `04`, `09`: `LAB1_DOOR2B`
  - Parts `05`, `10`: `LAB1_DOOR2A`
- Combined OBJ bounds: `(743, -241, -395)` to `(775, -113, -235)`
- Motion: vertical along `-Y` by `2.6` world units

## BlastDoor2

- Original export groups: `Brush1017` through `Brush1028`
- Renamed groups: `BlastDoor2_01` through `BlastDoor2_12`
- Materials:
  - Parts `01–02`, `07–08`: `LAB1_DOOR2A`
  - Parts `03–06`, `09–12`: `C1A1DOOREDGE`
- Combined OBJ bounds: `(296, -240, -396)` to `(328, -112, -236)`
- Motion: vertical along `-Y` by `2.6` world units

## BabtechDoor

- Original export groups: `Brush897`, `Brush900`, `Brush903`, `Brush904`,
  `Brush907`
- Renamed groups: `BabtechDoor_01` through `BabtechDoor_05`
- Material for every part: `BABTECH_DR1E`
- Combined OBJ bounds: `(-3289, -492, 1321)` to `(-3225, -396, 1345)`
- Motion: vertical along `-Y` by `2.2` world units

## LargeDoor

- Original export groups:
  - `Brush2339` → `LargeDoor_01` (`C1A1_DR3B`)
  - `Brush2550` → `LargeDoor_02` (`C1A1_DR3`)
- Combined OBJ bounds: `(-3073, -504, 1449)` to
  `(-3049, -360, 1641)`
- Motion: vertical along `-Y` by `3.2` world units

## TramDoor

- Stored separately in `assets/OBJ/tramDoor.obj`
- Group prefix: `TramDoor`
- Its closed transform is the final transform of the tram intro.

When brush numbers change after another export, identify each assembly using
the material combination and approximate bounds rather than relying only on
the old brush numbers.
