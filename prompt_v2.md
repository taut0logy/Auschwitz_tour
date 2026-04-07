# Auschwitz I — Interactive 3D Educational Reconstruction
## OpenGL / GLFW / GLAD / GLM — Enhanced Implementation Prompt
### Version 2.0 — Full Detail Specification

---

# 0. Project Overview & Goals

This document is the complete, consolidated implementation prompt for a coding agent tasked with building an interactive 3D educational reconstruction of Auschwitz I (Stammlager) using OpenGL Core Profile 3.3+, GLFW, GLAD, and GLM. The project is an academic exercise demonstrating mastery of 3D modelling, transformations, curved surfaces, texture mapping, Phong shading, multi-source lighting, shadow mapping, fractals, and particle systems.

The project is NOT a game. It is a respectful, educational 3D walkthrough with rich environmental detail, carefully authored lighting, and historically representative geometry.

## 0.1 Technology Stack

- **Language:** C++
- **Windowing:** GLFW 3.x
- **OpenGL loader:** GLAD (OpenGL 3.3 Core Profile)
- **Math:** GLM
- **Image loading:** stb_image.h (single-header, extern/stb/)
- **Build:** Visual Studio 2026

## 0.2 Core Required Features

| Feature | Requirement | Where Applied |
|---|---|---|
| 3D Modelling & Transformations | Cubes, cylinders, polygons, spheres | Buildings, towers, gate, fence, lamps |
| Bezier / Spline Curves | Curved surfaces, ruled surfaces | Gate arch, road paths, fence wire |
| Texture Mapping | 2D textures, mipmapping, wrapping, filtering | All surfaces |
| Phong Shading | Per-fragment vertex & fragment shaders | All lit surfaces |
| Multi-source Lighting | Point, directional, spotlight | Sun, moon, lamps, tower searchlights, interior |
| Illumination | Ambient, diffuse, specular interactions | All materials |
| Fractals | L-system trees, Koch snowflake particles | Tree rows, night snowfall |
| Day/Night Cycle | Sky gradient, sun/moon orbit, light transitions | Entire scene |
| Flyweight Pattern | Shared VAO/VBO instancing | Barracks, fence posts, lamps, towers |
| Horizon LOD | Layered parallax backdrop + mid-ground treeline | Scene edge |

---

# 1. World Coordinate System & Scale

**1 world unit = 1 metre throughout.** The camp interior spans approximately 270 m (X-axis, East-West) by 210 m (Z-axis, North-South) at this scale, representing the approximate 270 × 210 m footprint of Auschwitz I's inner compound.

```
World origin (0, 0, 0) = centre of the Appellplatz (roll-call square).
X+  = East  (toward main gate)
X-  = West  (toward crematorium)
Y+  = Up
Z+  = South (toward south fence)
Z-  = North (toward north fence)

Scene bounds (explorable):
  X: [-145, +145]
  Z: [-110, +110]
  Y: [1.5, 10.0]  (camera clamp — ground to just above roofline)

Fence perimeter (inner fence):
  North: Z = -95
  South: Z = +95
  West:  X = -135
  East:  X = +135
```

---

# 2. Precise Layout & Zone Coordinates

## 2.1 Barrack Grid — Block Positions (28 Blocks)

Each barrack block is **40 m long × 12 m wide × 8 m tall** (two storeys), with a **shallow gabled roof** rising an additional 2.5 m at the ridge. The long axis of each block runs **East-West (along X)**. Blocks are arranged in **4 rows × 7 columns**.

**Inter-block spacing:**
- Between columns (X-axis gap between block ends): **8 m** (this is the inter-block road)
- Between rows (Z-axis gap between block sides): **10 m** (primary road width: 6 m road + 2 m verge each side)
- Row Z-centres: Z = -52, -22, +18, +48 (rows north to south, approximately)
- Column X-centres: X = -80, -32, -16, 0, +16, +32, +48 — **RECALCULATED** as follows:

```
Column spacing = block_length(40) + road_width(8) = 48 m per column.
7 columns centred symmetrically on X = -15 to X = +72 (shifted east to leave
room for the Appellplatz and admin area to the west).

Column X-centres:
  Col 1: X = -72
  Col 2: X = -24
  Col 3: X = +24  ← Note: road gap between col2 and col3 is the Appellplatz cross-road
  Col 4: X = +48  (shifted +24 from col3 — wider gap here for Appellplatz)
  Col 5: X = +72
  Col 6: X = +96
  Col 7: X = +120

Row Z-centres:
  Row A (northernmost): Z = -54
  Row B:                Z = -28
  Row C:                Z = +2
  Row D (southernmost): Z = +28

Block numbering (historically approximate, not exact):
  B1–B7   = Row A, cols 1–7
  B8–B14  = Row B, cols 1–7
  B15–B21 = Row C, cols 1–7
  B22–B28 = Row D, cols 1–7

  Block 11 = Row B, Col 1 (X=-72, Z=-28) — special dark variant
  Block 10 = Row B, Col 2 (X=-24, Z=-28) — special dark variant
```

**Critical:** No road quad may overlap a barrack footprint. Roads run in the 8 m gap between block long-ends (X-direction) and the 10 m gap between block sides (Z-direction). Road surfaces sit at Y = 0.0, block foundations at Y = 0.0 with walls starting at Y = 0.

## 2.2 Road Network — Precise Geometry

**Lagerstrasse (main East-West road):** Runs at Z = -68 (just north of the northernmost block row), from X = -130 to X = +130. Width: 7 m. Surface: dirt_road.png.

**North-South inter-row roads (between each row pair):**
- Road 1 (between Row A and Row B): Z_centre = (-54 + (-28)) / 2 − 6 = **Z = -47**, width 10 m, runs X = -130 to +130
- Road 2 (between Row B and Row C): Z_centre = (-28 + 2) / 2 = **Z = -13**, width 10 m
- Road 3 (between Row C and Row D): Z_centre = (2 + 28) / 2 = **Z = +15**, width 10 m
- South verge road (south of Row D): Z_centre = **+44**, width 6 m

**East-West inter-column roads (between each column pair):** Run at the midpoints between column X-centres, in the Z range covering the 4 barrack rows (-65 to +38), width 8 m.

**Appellplatz (Roll-Call Square):** Centred at X = +36, Z = -13, dimensions 55 m × 90 m. Flat gravel surface (gravel.png). This large open space lies between the easternmost blocks and the main gate approach.

## 2.3 Main Entrance Gate — Zone E+

Located at X = +135, Z = 0. The gate complex consists of:

- **Gatehouse building:** 18 m wide × 12 m deep × 5.5 m tall. The entry tunnel through the building is 4 m wide × 4.2 m tall, centred at Z = 0. The gatehouse straddles the entry road.
- **Entry road through gate:** 4 m wide, runs East-West at Z = 0.
- **Flanking sentry boxes:** 2 m × 2 m × 2.4 m booths, one on each side of the road (Z = +4 and Z = -4, X = +140).
- **Flagpoles:** Two steel poles, 8 m tall, 0.15 m diameter cylinder, at X = +132, Z = ±6.
- **Gate arch (Bezier):** Spans the tunnel opening. See Section 7.1 for exact control points.

## 2.4 Perimeter Fence System

**Inner fence:** Runs at X = ±133, Z = ±93. Concrete posts: 0.12 m diameter, 2.8 m tall, spaced every **3.5 m** along the fence line. Total posts ≈ 224.

**Outer fence:** Offset 2.0 m outward from inner fence (X = ±135, Z = ±95). Same post specification.

**Wire strands per bay (between two consecutive posts):** 5 strands per fence face, at heights Y = 0.5, 0.9, 1.4, 1.9, 2.5 m. Each strand droops to Y_mid − 0.12 m (gravity sag via Bezier). See Section 7.3.

**Guard towers — 12 total:**
- 4 corner towers: (+133, 0, +93), (+133, 0, -93), (-133, 0, +93), (-133, 0, -93)
- 8 mid-perimeter towers, evenly distributed along each long fence face at X-intervals of ~44 m and Z-intervals of ~46 m.
- Tower base centre is 2.5 m outside the outer fence.

## 2.5 Street Lamp Positions

Street lamps appear on: the Lagerstrasse, all inter-row roads, the Appellplatz perimeter, and near key buildings (gate, crematorium entrance, admin area).

**Lagerstrasse:** Lamps every **20 m**, alternating sides of the road (left/right), from X = -125 to +125 at Z = -68 ± 4.5 m (offset 4.5 m from road centreline).

**Inter-row roads:** Lamps every **24 m** along the road centreline, offset 3 m to one side.

**Appellplatz perimeter:** 8 lamps spaced around the perimeter, offset 3 m inward.

**Near gate:** 4 lamps within 15 m of the gatehouse entrance.

**Total lamps:** Approximately 80–90 instances. All share one Flyweight lamp mesh.

## 2.6 Crematorium I

Located at X = -118, Z = -30 (northwest corner of camp, set back from the barracks grid).

- **Main building:** 30 m long × 11 m wide × 4.5 m tall. Low-profile, single-storey.
- **Chimney stack:** 1.6 m × 1.6 m square cross-section, 12 m tall, positioned at X = -125, Z = -32 (western end of building).
- **Furnace antechamber extension:** A 6 m × 6 m × 3.5 m annex on the western end.
- **Small perimeter gravel path:** 2 m wide, surrounds the building.
- **Gallows:** Located at X = -110, Z = -22. See Section 3.5 for geometry.

## 2.7 Administrative Zone

Located north of the barracks grid, roughly at X = +20 to +80, Z = -80 to -68 (between the Lagerstrasse and north fence).

- **Commandant's office:** X = +65, Z = -80, 20 m × 10 m × 6 m, two storeys.
- **SS administration building:** X = +30, Z = -80, 24 m × 12 m × 6 m.
- **Kitchen building:** X = -10, Z = -80, 22 m × 10 m × 5 m, single-storey.
- **Reception/bathhouse:** X = +90, Z = -80, 18 m × 12 m × 5.5 m.

## 2.8 Block 11 Courtyard & Death Wall

Block 11 (X = -72, Z = -28) and Block 10 (X = -24, Z = -28) face each other across a courtyard enclosed by high walls.

- **Courtyard enclosure walls:** 3.5 m tall brick walls running between the facing ends of Block 10 and Block 11. Dimensions: 14 m × 14 m courtyard enclosed area, walls at X = -68, X = -28, Z = -34, Z = -22.
- **Death Wall:** A wooden shooting wall, 5 m wide × 3 m tall, positioned at Z = -22 (north end of courtyard), X = -48 (centred). Backed by a sand/gravel berm.
- **Wooden screens:** 2.5 m tall opaque wooden panels lining the courtyard walls (same wood_plank texture, different geometry from fence).
- **Gravel floor:** Courtyard surface is a separate gravel quad rendered at Y = 0.

---

# 3. Detailed 3D Object Specifications

## 3.1 Barrack Block — Exterior Geometry

Each standard barrack is built from the following sub-components, all placed relative to the block's local origin at its southwest corner base:

```
Block local origin: (0, 0, 0) = southwest corner at ground level.
Long axis: X (40 m). Width: Z (12 m). Height: 8 m walls + 2.5 m roof ridge.

SUB-COMPONENTS:
1. Main wall box:       40 × 8 × 12 (X × Y × Z). Brick texture UV-mapped.
   - UV scale: repeat every 0.5 m (so 80 repeats along 40 m, 16 along 8 m)

2. Gabled roof:
   - Two rectangular roof panels, each 40 × 0.3 m thick, pitched at 22.5° from
     horizontal, meeting at a ridge at Y = 10.5 m. Each panel is a quad with
     width = 12/cos(22.5°) = 6.5 m slant, length = 40 m.
   - Ridge beam: a 40 × 0.25 × 0.25 m box running along the top.
   - Two triangular gable end-caps (triangular polygons filling the gable gap).
   - Roof overhang: 0.5 m beyond wall faces on both long sides, 0.3 m on gable ends.

3. Windows (per storey, per long wall):
   - 7 windows per storey per long face (north and south faces) = 28 total.
   - Each window opening: 1.0 m wide × 1.2 m tall, recessed 0.15 m into the wall.
   - Window frame: 4 thin box strips (0.08 m wide) forming the frame geometry.
   - Window sill: a 0.1 m deep × 0.08 m tall ledge protruding from wall.
   - Glazing quad: a flat quad inside the recess, rendered with alpha glass shader
     (semi-transparent, slight tint, specular highlight).
   - Window positions X: evenly spaced, first at X_local = 2.5 m, then every 5.2 m.
   - Storey 1 windows: Y_centre = 2.0 m. Storey 2 windows: Y_centre = 5.5 m.
   - Gable end windows: 2 windows per gable, placed at X=±0 (end wall), at
     Y = 2.0 and Y = 5.5 m.

4. Door (ground floor, each gable end):
   - One door per short end (east and west walls).
   - Door opening: 1.2 m wide × 2.2 m tall, centred on the wall.
   - Door frame: thin box surround (0.1 m wide, 0.1 m deep).
   - Door leaf: flat quad with wood_dark texture.

5. Chimney stacks (one per block):
   - 0.6 × 0.6 m cross-section, 1.8 m tall above roof ridge, brick_dark texture.
   - Located at X_local = 4 m, Z_local = 6 m (on roof ridge).

6. Foundation ledge:
   - A 40.4 × 0.4 × 12.4 m box at Y = -0.4 to 0.0 (concrete base below brick),
     concrete.png texture.
```

**Special variants:**
- **Block 11 / Block 10:** Use `brick_dark.png`. Windows have external iron bars (6 vertical bar cylinders per window, 0.03 m radius, spanning the full window height).
- **Punishment cells (Block 11 ground floor):** 8 small windows 0.4 m × 0.3 m instead of standard windows on the north face.

## 3.2 Barrack Interiors — 3 Repeating Types

All 28 barracks have walkable, lit interiors. Interior geometry is distinct from exterior (interior walls, floor, ceiling rendered only when camera is inside or looking through a window). Use **2–3 interior types that repeat** across blocks.

### Interior Type A — Sleeping Quarters (Blocks 1–9, 12–21, 23–28)

The interior space is 38.5 m long × 11 m wide × 3.6 m clear height per storey (two storeys, with a floor/ceiling slab at Y = 3.8 m).

```
GROUND FLOOR:
1. Floor: flat quad, wood_plank.png, plank direction along X-axis.
2. Ceiling (underside of storey 2 slab): flat quad, plaster_white.png (or
   wood_plank.png for a rough ceiling), at Y = 3.8 m.
3. Interior walls: brick_interior.png (rougher, darker variant of brick).
4. Central support columns: 0.4 × 0.4 m brick columns at 5 m intervals
   along the centreline (X_local = 5, 10, 15, 20, 25, 30, 35). Y = 0 to 3.8 m.
5. Triple-tier bunk beds:
   - 20 bunk units per storey floor (10 on each side of the central aisle).
   - Each bunk unit: 2.0 m long × 0.9 m wide × 1.8 m tall (three tiers at
     Y = 0.3, 1.1, 1.8 m bed surface). Built from thin box planks.
   - Frame: 4 vertical posts (0.06 × 0.06 m, 1.9 m tall), 3 horizontal bed
     platforms (2.0 × 0.9 × 0.06 m each), 2 ladder rungs on one end (0.04 m
     diameter cylinders, spaced 0.35 m apart vertically).
   - Placed against north and south walls: Z_local = 0.5 (north row) and
     Z_local = 8.5 (south row), spaced 1.9 m apart in X.
   - Bunk texture: wood_plank.png with a straw_bedding.png overlay quad on
     each sleeping surface.
6. Central aisle: 2.5 m wide (Z = 4 to 5.5 m in local coords). Bare floor.
7. Heating stove (1 per storey): a 0.8 × 1.2 × 0.8 m (W×H×D) brick box,
   black_metal.png, at X_local = 20 m, Z_local = 6 m (beside the aisle).
   - Firebox door: small 0.25 × 0.2 m recessed quad.
   - Emissive orange glow: a point light at stove centre, warm orange
     (#FF6A00), intensity 0.6, attenuation (1.0, 0.14, 0.07). Active always.
   - Emissive quad on firebox door face simulates fire glow.
8. Hanging bare light bulbs (per storey):
   - 4 bulbs strung along the central aisle at X = 8, 16, 24, 32 m,
     Y = 3.5 m (just below ceiling).
   - Each bulb: a 0.04 m radius sphere with unlit emissive shader (#FFEE88).
   - Pendant cord: a GL_LINES segment from ceiling to bulb, 0.003 m radius
     thin cylinder.
   - Point light at each bulb: warm white (#FFF5CC), intensity 1.2,
     attenuation (1.0, 0.22, 0.20) → illuminates ~ 4 m radius.
9. Small wooden table and benches near stove:
   - Table: 1.5 × 0.75 × 0.7 m box (tabletop 0.05 m thick on 4 legs 0.06 m sq).
   - 2 benches: 1.2 × 0.25 × 0.4 m, leg detail optional.
10. Cobweb corner decals (optional): dark thin GL_LINE geometry.
11. Second storey: identical layout, accessed via a staircase at X_local = 37 m
    (near east gable). Staircase: 6 steps, each 0.25 m rise × 0.3 m tread,
    box geometry, wood_plank.png.
```

### Interior Type B — Washroom / Utility Block (Blocks 10, 22)

```
Single-storey-height interior (3.8 m), concrete and tile aesthetic.
Floor: tile_grey.png. Walls: tile_white.png (lower 1.5 m), plaster above.
1. Long concrete trough along centreline: 35 m × 0.5 m × 0.8 m box,
   concrete.png. 10 tap fixtures spaced every 3 m (small cylinder + elbow
   box = tap assembly, metal_iron.png).
2. Open latrine benches along south wall: long wooden bench with round holes.
3. 2 hanging bulb lights (same spec as Type A but dimmer, intensity 0.8).
4. Small frosted window glazing quads (same window slots as exterior).
```

### Interior Type C — Block 11 Punishment Block

```
Darker and more oppressive. brick_dark.png throughout. Very low ambient.
Ground floor:
1. Corridor: 1.5 m wide along the centreline. Dark stone_floor.png.
2. Cells: 8 cells per side (16 total), each 2.5 m × 2.5 m × 2.8 m tall.
   - Cell walls: brick_dark.png.
   - Cell door: heavy wood frame (wood_dark.png), 1.0 m wide × 2.0 m tall,
     with a small barred viewport 0.2 m × 0.15 m at eye height.
   - Standing cell (cells 1–4 on north side): only 0.9 m × 0.9 m footprint.
3. No bunk beds. Single straw pile quad on floor.
4. No overhead lights except one dim bulb at corridor midpoint (intensity 0.4).
5. A single point light per stove location is replaced by a near-black ambient
   multiplier (0.02 for this zone).
6. Barred windows from exterior are visible as small light slots — a very dim
   directional beam (like a shaft of light from window) using a narrow spotlight
   aimed inward during daytime.
```

## 3.3 Guard Towers — Detailed Geometry

Each of the 12 guard towers is built from the following components in local coords (origin at base centre):

```
1. Foundation pad: 4.5 × 4.5 × 0.3 m concrete box at Y = -0.3 to 0.0.
   concrete.png.

2. Four corner support legs:
   - Cylinder, radius 0.15 m, height 4.8 m, vertical.
   - Positions: (±1.6, 0, ±1.6) in local XZ.
   - Texture: wood_plank.png.

3. Cross-bracing (X-pattern between legs, 2 heights):
   - At Y = 1.5 m and Y = 3.0 m: 4 diagonal thin cylinders (radius 0.06 m)
     connecting adjacent leg tops. Computed length from geometry.
   - wood_plank.png.

4. Platform floor:
   - 4.8 × 4.8 × 0.2 m box at Y = 4.8 to 5.0. wood_plank.png.

5. Platform railing:
   - 4 rail walls around the platform perimeter, each 0.08 m thick × 0.9 m tall.
   - 3 horizontal rails per face (box strips 0.06 × 0.06 m) at Y = 5.3, 5.7, 5.9 m.
   - 5 vertical balusters per face (cylinder r=0.03 m). wood_plank.png.

6. Cabin (on platform):
   - 3.0 × 2.4 × 2.2 m box at Y = 5.0 to 7.2. wood_plank.png.
   - 2 window openings per face: 0.5 m × 0.5 m, no glazing (open slots).
   - Door: 0.7 m × 1.8 m opening on one face.

7. Roof:
   - Low hip roof: 4 triangular panels rising to a ridge point at Y = 8.0 m.
   - Overhang: 0.4 m beyond cabin walls on all sides.
   - roof_tile.png.

8. Searchlight assembly (top of cabin, exterior):
   - Housing box: 0.4 × 0.3 × 0.5 m, metal_grey.png, at Y = 7.2 m.
   - Lens disc: a flat circle (16-segment polygon) 0.3 m radius, emissive white
     when active (night), dark when off (day). Mounted on front face of housing.
   - The OpenGL spotlight for this tower emanates from this lens position.
   - Slow sweep: lens housing rotates on Y-axis at 0.2 rad/s unique per tower.

9. Access ladder:
   - Vertical cylinder pair (rails, r=0.03 m) from Y=0 to Y=4.8 m at
     X_local = 1.9 m (adjacent to one leg).
   - Rungs: horizontal cylinders r=0.02 m, every 0.35 m height, 0.3 m long.
   - wood_dark.png.
```

## 3.4 Street Lamp — Detailed Geometry

Each lamp shares one Flyweight mesh. Local origin at base.

```
1. Pole: cylinder, r = 0.06 m, height = 5.5 m. metal_grey.png.

2. Pole top curve:
   - A Bezier-extruded arm: the lamp arm extends 1.2 m horizontally then
     curves down 0.4 m. Cross-section circle r = 0.04 m.
   - Control points (in local XZ plane at Y = 5.5 m):
     P0 = (0, 5.5), P1 = (0.6, 5.5), P2 = (1.2, 5.2), P3 = (1.2, 5.1)
   - Extrude circle cross-section along this curve (16 segments along curve,
     8-sided cross-section circle). metal_grey.png.

3. Lamp housing:
   - Truncated cone: top radius 0.18 m, bottom radius 0.08 m, height 0.22 m.
     At Y = 5.1 m (hanging below arm tip). metal_grey.png.
   - Open bottom face.

4. Bulb:
   - Sphere r = 0.07 m, emissive shader. Colour (#FFEEAA) always on.
   - Positioned at Y = 4.97 m (inside housing cone, slightly below).
   - This sphere is rendered with the UNLIT/EMISSIVE shader — no lighting
     calculation. Always bright regardless of day/night.

5. Light source:
   - A SpotLight positioned at the bulb centre.
   - Direction: (0, -1, 0) (straight down).
   - Inner cone: 35°. Outer cone: 50°.
   - Colour: warm white (#FFF0CC).
   - Intensity: 1.8 (night) / 0.0 (day — lamps off during day).
   - Attenuation: constant=1.0, linear=0.09, quadratic=0.032.
   - This produces a realistic circular pool of warm light on the ground (~5 m
     diameter lit area).
   - Lamps activate when timeOfDay < 6.5 or timeOfDay > 19.0.
```

**Flyweight note:** All ~85 lamps share one VAO. Each instance has its own model matrix (translation + rotation to orient arm toward road centreline) and is rendered in a single loop with `shader.setMat4("model", instance.model)`.

## 3.5 Gallows Geometry

Located at X = -110, Z = -22, Y = 0.

```
1. Two vertical posts: box 0.2 × 0.2 × 3.0 m, wood_dark.png. At X = ±1.0 m.
2. Horizontal crossbeam: box 2.4 × 0.2 × 0.2 m at Y = 2.8 m.
3. Rope: thin cylinder r = 0.02 m, hanging 0.8 m down from crossbeam centre.
   - Noose: a torus (r_major = 0.08 m, r_minor = 0.015 m) at rope bottom.
   - wood_dark.png / rope.png.
4. Platform step: 1.0 × 0.25 × 0.6 m box at base of posts, Y = 0 to 0.25 m.
```

## 3.6 Entrance Gate — Detailed Geometry

The gatehouse building and gate arch are the most complex objects in the scene.

```
GATEHOUSE BUILDING (centred at X=135, Z=0, origin at south-west corner base):
  Width (Z): 18 m. Depth (X): 12 m. Height: 5.5 m walls.
  Divided into three sections by the entry tunnel:

  Section A (south wing): Z = +2 to +9 m from building south face.
    - Solid box, 12 × 5.5 × 7 m. brick_dark.png.
    - 4 windows on south face, 2 on east face.
    - Interior: a guardroom. 1 table, 4 chairs (box geometry), 2 hanging bulbs.

  Entry tunnel: Z = -2 to +2 m (4 m wide, 4.2 m tall arch opening).
    - Tunnel ceiling: a barrel vault (half-cylinder, r = 2.1 m, length 12 m).
      Generated by extruding a half-circle (16 segments) along the depth axis.
    - Tunnel walls: brick_dark.png.
    - Tunnel floor: flat quad, cobblestone.png.

  Section B (north wing): Z = -9 to -2 m from building south face.
    - Mirror of Section A.

  Roof: low hipped roof over each wing section, roof_tile.png, ridge at Y=7.5 m.
  No roof over tunnel span (open to sky above arch).

GATE ARCH (Bezier) — "Arbeit Macht Frei" text:
  The arch is the iconic wrought-iron gate spanning the tunnel entrance.
  It sits at X = +135.5 (just outside the east face of gatehouse), Z = -2 to +2 m.

  Arch frame geometry (see Section 7.1 for Bezier details):
  - Left vertical bar: box 0.08 × 0.08 m cross-section, height 3.5 m. At Z = -1.8 m.
  - Right vertical bar: same, at Z = +1.8 m.
  - Arch bar: Bezier-extruded tube (r = 0.06 m) spanning between bar tops.
  - 3 horizontal intermediate bars across the arch opening (at Y = 1.0, 2.0, 3.0 m).
  - Vertical decorative bars: 12 thin bars (r=0.015 m) spanning from bottom
    horizontal to arch tube, evenly spaced.
  - ALL metal_iron.png. High specular (see material table).

  "Arbeit Macht Frei" text:
  - Rendered as a texture billboard attached to the arch horizontal bar at Y = 3.0 m.
  - A flat quad, 3.2 m wide × 0.35 m tall, with text_arbeit.png (pre-made RGBA
    texture with white letters on transparent background).
  - Rendered with alpha blending. No backface.

  Gate leaves (two hinged panels):
  - Each panel: 1.8 m wide × 3.4 m tall, made of a grid of 0.06 × 0.06 m box
    strips forming a rigid iron grid pattern (4 horizontal × 8 vertical members).
  - Hinge cylinders (r=0.04, h=0.12) at Y = 0.5, 1.7, 2.9 m on each vertical bar.
  - metal_iron.png.
  - Gates are rendered in the "open" position (rotated ~90° on Y-axis, flush with wall).
```

---

# 4. Shader System

## 4.1 Shaders

| Shader | Files | Purpose |
|---|---|---|
| Main Phong | shaders/phong.vert + phong.frag | All standard scene objects |
| Unlit/Emissive | shaders/unlit.vert + unlit.frag | Bulb spheres, sun, moon |
| Particle | shaders/particle.vert + particle.frag | Chimney smoke, snowflakes |
| Skybox | shaders/skybox.vert + skybox.frag | Procedural sky gradient |
| Alpha-blend | shaders/alpha.vert + alpha.frag | Fence wire, leaves, horizon, glass |
| Shadow depth | shaders/shadowmap.vert + shadowmap.frag | Depth-only shadow pass |
| Interior | shaders/interior.vert + interior.frag | Interior walls, rendered back-face only |

## 4.2 Phong Fragment Shader — Full Light Loop

```glsl
// phong.frag — GLSL 330 core

#define MAX_POINT_LIGHTS    32   // building interiors (4 per barrack × 28 + extras)
#define MAX_SPOT_LIGHTS     110  // 12 towers + ~85 street lamps + 4 gate spots
#define MAX_DIR_LIGHTS       2   // sun + moon

struct Material {
    sampler2D diffuse;
    sampler2D specular;  // optional spec map; use uniform vec3 if absent
    float shininess;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
};

struct DirLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant, linear, quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float constant, linear, quadratic;
    float cutOffCos;       // cos(innerAngle)
    float outerCutOffCos;  // cos(outerAngle)
};

uniform Material material;
uniform DirLight  dirLights[MAX_DIR_LIGHTS];
uniform int       numDirLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int        numPointLights;
uniform SpotLight  spotLights[MAX_SPOT_LIGHTS];
uniform int        numSpotLights;
uniform vec3  viewPos;
uniform vec3  fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform sampler2D shadowMap;
uniform mat4  lightSpaceMatrix;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

out vec4 FragColor;

float shadowFactor(vec4 fragPosLS) {
    vec3 proj = fragPosLS.xyz / fragPosLS.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;
    float bias = 0.005;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y) {
            float depth = texture(shadowMap, proj.xy + vec2(x,y)*texelSize).r;
            shadow += (proj.z - bias > depth) ? 1.0 : 0.0;
        }
    return shadow / 9.0;
}

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 texColor) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 ambient  = material.ambientStrength  * light.color * texColor;
    vec3 diffuse  = material.diffuseStrength  * diff * light.color * texColor;
    vec3 specular = material.specularStrength * spec * light.color;
    return (ambient + (1.0 - shadowFactor(FragPosLightSpace)) * (diffuse + specular))
           * light.intensity;
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 texColor) {
    vec3 lightDir = normalize(light.position - FragPos);
    float dist = length(light.position - FragPos);
    float atten = 1.0 / (light.constant + light.linear*dist + light.quadratic*dist*dist);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 ambient  = material.ambientStrength  * light.color * texColor;
    vec3 diffuse  = material.diffuseStrength  * diff * light.color * texColor;
    vec3 specular = material.specularStrength * spec * light.color;
    return (ambient + diffuse + specular) * atten * light.intensity;
}

vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 viewDir, vec3 texColor) {
    vec3 lightDir = normalize(light.position - FragPos);
    float theta   = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOffCos - light.outerCutOffCos;
    float intensity = clamp((theta - light.outerCutOffCos) / epsilon, 0.0, 1.0);
    float dist = length(light.position - FragPos);
    float atten = 1.0 / (light.constant + light.linear*dist + light.quadratic*dist*dist);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 ambient  = material.ambientStrength  * light.color * texColor;
    vec3 diffuse  = material.diffuseStrength  * diff * light.color * texColor;
    vec3 specular = material.specularStrength * spec * light.color;
    return (ambient + (diffuse + specular) * intensity) * atten * light.intensity;
}

void main() {
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 texColor = vec3(texture(material.diffuse, TexCoords));

    vec3 result = vec3(0.0);
    for (int i = 0; i < numDirLights;   ++i) result += calcDirLight(dirLights[i], norm, viewDir, texColor);
    for (int i = 0; i < numPointLights; ++i) result += calcPointLight(pointLights[i], norm, viewDir, texColor);
    for (int i = 0; i < numSpotLights;  ++i) result += calcSpotLight(spotLights[i], norm, viewDir, texColor);

    // Distance fog
    float dist      = length(FragPos - viewPos);
    float fogFactor = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);
    result = mix(fogColor, result, fogFactor);

    FragColor = vec4(result, 1.0);
}
```

**Implementation note:** Because MAX_SPOT_LIGHTS = 110 exceeds what many drivers efficiently support in a single draw call, use **light culling**: before each frame, build a list of spotlights whose bounding sphere intersects the camera frustum, capped at the top 24 nearest lamps + all 12 tower spots. Only pass these to the shader. Street lamps beyond 60 m from the camera are excluded.

---

# 5. Lighting System

## 5.1 Complete Light Inventory

| Light ID | Type | Colour | Intensity Day | Intensity Night | Notes |
|---|---|---|---|---|---|
| Sun | Directional | #FFF5E0 | 1.0 | 0.0 | Arc across sky, casts shadows |
| Moon | Directional | #C8D8FF | 0.0 | 0.18 | Opposite arc to sun |
| Global ambient | Ambient | scene sky horizon | 0.30 | 0.04 | Smooth transition |
| Tower searchlight ×12 | Spotlight | #FFFFE0 | 0.0 | 2.5 | Sweeping, narrow cone 18°/30° |
| Street lamp ×85 | Spotlight downward | #FFF0CC | 0.0 | 1.8 | Active at night, pool on ground |
| Barrack bulb ×4/block | Point light | #FFF5CC | 0.6 | 1.0 | Interior per-storey |
| Barrack stove ×1/block | Point light | #FF6A00 | 0.6 | 0.7 | Orange, low reach |
| Crematorium ground | Point light | #FF3300 | 0.3 | 0.5 | Near chimney base, red-orange |
| Gate entry sentry | Spotlight ×2 | #FFEECC | 0.0 | 1.5 | Aimed at tunnel entrance |
| Death Wall | Point light | #220000 | 0.0 | 0.05 | Near darkness, oppressive |
| Admin area lamps ×4 | Point light | #FFEECC | 0.5 | 0.9 | Near building entrances |

## 5.2 Tower Searchlight — Sweep Behaviour

```cpp
// Each tower has a unique phase offset so they don't sweep in sync
float phase = towerIndex * (2.0f * PI / 12.0f);
float sweepAngle = glfwGetTime() * 0.25f + phase;

// Sweep cone: inner 18°, outer 30°, range 80 m
// Aim slightly down: direction = normalize(vec3(cos(sweepAngle), -0.6f, sin(sweepAngle)))
// Active only when timeOfDay < 6.5 || timeOfDay > 19.0
// Transition: smoothstep fade over 20 min of in-game time
```

## 5.3 Shadow Mapping

- **Shadow map resolution:** 4096 × 4096 depth texture for sharper shadows given the large scene.
- **Orthographic projection:** left=-140, right=140, bottom=-110, top=110, near=-20, far=200 — covers full scene from sun direction.
- **PCF kernel:** 5×5 sample grid for soft edges.
- **Shadow bias:** Slope-scale depth bias (`glPolygonOffset(2.0f, 4.0f)` in shadow pass) to eliminate shadow acne on sloped roof surfaces.
- **Only the directional sun light casts shadows.** Tower spots and street lamps do not (performance).

---

# 6. Day/Night Cycle

## 6.1 Sky Gradient Keyframes

The skybox hemisphere shader uses smooth cubic interpolation (smoothstep) between these keyframes, evaluated per-fragment based on `timeOfDay` and fragment elevation angle.

```glsl
// skybox.frag — evaluate sky color based on elevation and time
// horizon color and zenith color are passed as uniforms, interpolated on CPU

uniform vec3 zenithColor;
uniform vec3 horizonColor;
uniform float sunGlowStrength;
uniform vec3 sunGlowDir;  // sun direction in world space

void main() {
    vec3 dir = normalize(texCoord); // direction to sky fragment
    float t = clamp(dir.y, 0.0, 1.0); // elevation factor
    vec3 baseColor = mix(horizonColor, zenithColor, pow(t, 0.5));

    // Sun/moon glow halo
    float glowDot = max(dot(dir, sunGlowDir), 0.0);
    float glow = pow(glowDot, 32.0) * sunGlowStrength;
    baseColor += vec3(1.0, 0.8, 0.4) * glow;

    FragColor = vec4(baseColor, 1.0);
}
```

**CPU keyframe table** (DayNightCycle.cpp interpolates between these):

| Hour | Zenith | Horizon | Fog/Ambient |
|---|---|---|---|
| 0.0 | #080818 | #0D0D25 | fog=#0D0D20 |
| 5.5 | #1A1540 | #3D2255 | fog=#221440 |
| 6.5 | #FF5500 | #FFB347 | fog=#FFAA44 |
| 8.0 | #6BAED6 | #C6E2F0 | fog=#B0D8F0 |
| 12.0 | #2171B5 | #9DC9E8 | fog=#A8CEE8 |
| 17.0 | #F16913 | #FEC44F | fog=#FDBE4A |
| 19.5 | #A63603 | #D94801 | fog=#C44000 |
| 21.0 | #2D1040 | #150A25 | fog=#100820 |
| 24.0 | #080818 | #0D0D25 | fog=#0D0D20 |

## 6.2 Sun & Moon Geometry

Both are rendered as unlit emissive spheres with the `unlit` shader. No lighting is computed for them; they always display at full brightness.

```cpp
// Sun sphere: r = 8.0 m, placed 600 m from origin
float sunAngle = ((timeOfDay / 24.0f) * 2.0f * PI) - PI * 0.5f; // noon = zenith
glm::vec3 sunPos = glm::vec3(cos(sunAngle) * 600.0f,
                              sin(sunAngle) * 400.0f,
                              -100.0f);
// Sun color transitions from deep orange at dawn to bright white at noon
// Moon sphere: r = 5.0 m, offset PI from sun, pale blue-grey
```

## 6.3 Stars (Night Only)

Render 800 stars as point sprites (`GL_POINTS`, size 1.5–3.0 px varied by brightness). Each star is a fixed world-space point on a 600 m hemisphere. Stars fade in with `alpha = smoothstep(20.5, 22.0, timeOfDay)` and fade out at dawn. Use the `particle` shader with a tiny white circle texture. No depth write.

---

# 7. Bezier Curves & Splines

## 7.1 Gate Arch — Precise Control Points

The arch bar is a cubic Bezier extruded with a circular cross-section (r = 0.06 m, 8-sided polygon):

```
Arch spans from Z = -1.8 m to Z = +1.8 m at Y = 3.5 m (top of vertical bars).
The curve is defined in the YZ plane (gate faces X-axis):

P0 = (Y=3.5, Z=-1.8)  // left top of vertical bar
P1 = (Y=5.2, Z=-1.8)  // left control — rises sharply, no Z change
P2 = (Y=5.2, Z=+1.8)  // right control — same height, crosses to right
P3 = (Y=3.5, Z=+1.8)  // right top of vertical bar

Sample: 32 segments along the curve.
For each pair of adjacent sample points, generate a quad strip
connecting the 8-sided cross-section circles at each point.
Total triangles for arch: 32 × 8 × 2 = 512 triangles.
```

## 7.2 Lamp Arm — Bezier Extrusion

```
Lamp arm is a smaller extruded Bezier (local space, origin at pole top):
P0 = (0, 0, 0)
P1 = (0.5, 0, 0)
P2 = (1.0, -0.1, 0)
P3 = (1.2, -0.4, 0)
Cross-section: circle r = 0.035 m, 6-sided polygon. 12 segments along curve.
```

## 7.3 Fence Wire — Droop Bezier

```
For a wire strand between post A at (Ax, Wy, Az) and post B at (Bx, Wy, Bz):
  Droop amount = 0.10 m
  P0 = (Ax, Wy, Az)
  P1 = (Ax + (Bx-Ax)*0.33, Wy - droop, Az + (Bz-Az)*0.33)
  P2 = (Ax + (Bx-Ax)*0.67, Wy - droop, Az + (Bz-Az)*0.67)
  P3 = (Bx, Wy, Bz)

Render: 16 sample points, GL_LINE_STRIP with wire_alpha.png applied as
a tube texture. Alternatively, build a thin tube mesh (4-sided, r=0.008 m).

5 strands per bay at heights: 0.5, 0.9, 1.4, 1.9, 2.5 m above ground.
```

## 7.4 Roads — Catmull-Rom

Inter-row roads are straight but their edges use Catmull-Rom splines at intersections to produce smooth T-junction curves rather than sharp corners:

```cpp
// At each T-junction, blend a 0.5 m radius corner using 8 spline samples
// Road quad width: see Section 2.2
// Road surface sits at Y = 0.005 (slightly above ground to avoid Z-fighting)
// Use a separate dirt_road.png UV for roads vs gravel.png for camp ground
```

## 7.5 Crematorium Roof — Ruled Surface

```
The crematorium low roof is generated as a ruled surface between two parallel Bezier curves
(one at each gable end of the building, Z = -5.5 and Z = +5.5 in local coords):

Each gable curve (XY plane):
  P0 = (-15, 4.5)   // left eave
  P1 = (-5, 5.5)    // left slope control
  P2 = (+5, 5.5)    // right slope control
  P3 = (+15, 4.5)   // right eave

Ruled surface: for each pair of corresponding parameter values t∈[0,1],
connect point on curve A to point on curve B with a straight quad strip.
32 divisions along U (t parameter), 1 division along V (the ruling).
This produces a slightly curved saddle-roof with 32 quad strips.
```

---

# 8. Texture Mapping

## 8.1 Texture Asset List

| Filename | Format | Dimensions | Applied To | Wrap | Filter |
|---|---|---|---|---|---|
| brick_red.png | RGB | 512×512 | Barrack walls | REPEAT | Mipmap linear |
| brick_dark.png | RGB | 512×512 | Block 11, Crematorium, courtyard | REPEAT | Mipmap linear |
| brick_interior.png | RGB | 512×512 | Interior walls (rougher) | REPEAT | Mipmap linear |
| roof_tile.png | RGB | 512×256 | All roofs | REPEAT | Mipmap linear |
| concrete.png | RGB | 256×256 | Posts, foundation, road borders | REPEAT | Mipmap linear |
| gravel.png | RGB | 512×512 | Ground plane (main) | REPEAT | Anisotropic + mipmap |
| dirt_road.png | RGB | 256×512 | Roads | REPEAT | Mipmap linear |
| wood_plank.png | RGB | 256×512 | Towers, bunks, platforms | REPEAT | Mipmap linear |
| wood_dark.png | RGB | 256×256 | Doors, Death Wall, dark wood | REPEAT | Mipmap linear |
| metal_iron.png | RGB | 256×256 | Gate ironwork | CLAMP | Linear |
| metal_grey.png | RGB | 256×256 | Lamp poles, searchlight housing | REPEAT | Mipmap linear |
| wire_alpha.png | RGBA | 64×64 | Barbed wire strands | CLAMP | Linear |
| bark.png | RGB | 256×512 | Tree trunks | REPEAT | Mipmap linear |
| leaf_alpha.png | RGBA | 128×128 | Tree leaf billboards | CLAMP | Linear |
| glass_alpha.png | RGBA | 64×64 | Window glazing (slight tint) | CLAMP | Linear |
| moon.png | RGBA | 256×256 | Moon sphere | CLAMP | Linear |
| fractal_snowflake.png | RGBA | 64×64 | Snowflake particles (generated at runtime) | CLAMP | Linear |
| horizon_near.png | RGBA | 2048×256 | Mid-ground treeline mesh | CLAMP | Linear |
| horizon_far.png | RGBA | 4096×256 | Far billboard horizon | CLAMP | Linear |
| sky_star.png | RGBA | 8×8 | Star point sprites | CLAMP | Linear |
| straw_bedding.png | RGB | 128×128 | Bunk sleeping surface | REPEAT | Linear |
| stone_floor.png | RGB | 256×256 | Block 11 corridor floor | REPEAT | Mipmap linear |
| cobblestone.png | RGB | 256×256 | Gate tunnel floor | REPEAT | Mipmap linear |
| tile_grey.png | RGB | 256×256 | Washroom floor | REPEAT | Mipmap linear |
| plaster_white.png | RGB | 256×256 | Ceilings | REPEAT | Mipmap linear |
| text_arbeit.png | RGBA | 512×64 | Gate arch text billboard | CLAMP | Linear |
| rope.png | RGB | 64×128 | Gallows rope | REPEAT | Linear |
| black_metal.png | RGB | 128×128 | Stove box | REPEAT | Linear |

## 8.2 UV Mapping Strategy

For every object, UV coordinates are explicitly assigned to maintain correct texel density:
- **Brick walls:** 1 UV unit = 0.5 m. A 40 m wall → U repeats 80 times.
- **Roof tiles:** 1 UV unit = 1.0 m.
- **Ground plane:** 1 UV unit = 2.0 m (large area, tiles frequently).
- **Lamp poles:** U wraps around circumference (one full revolution = U=0 to U=1), V scales with height.
- **Window glass:** Clamp to edge, covering just the window quad, slight offset in each instance for variation.

## 8.3 Anisotropic Filtering

Apply to: `gravel.png`, `dirt_road.png`, `brick_red.png`, `brick_dark.png`. These are viewed at extreme grazing angles.

```cpp
float maxAniso;
glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, min(maxAniso, 16.0f));
```

---

# 9. Horizon & Environment — Layered Parallax

The area beyond the perimeter fence uses a three-layer parallax system to avoid an empty void at the scene edge:

## 9.1 Layer 1 — Ground Extension Plane

A large flat quad extends from the fence perimeter outward 200 m in all directions, at Y = -0.1 (slightly below camp interior to avoid z-fighting at fence line). Texture: a blended mix of `gravel.png` and a slightly greener `field_dirt.png`. Fog begins at distance 90 m from camera and completely obscures this layer by 160 m.

## 9.2 Layer 2 — Mid-Ground Treeline Mesh (50 m beyond fence)

A row of simplified tree silhouette meshes placed 50 m outside the fence, forming a continuous band around all four sides. These are NOT L-system trees — they are flat alpha-blended quad pairs (two crossed planes, same as billboard tree technique) at intervals of 5 m, varying height 8–14 m.

```
Texture: horizon_near.png (RGBA, 2048×256 — a stylised dark treeline silhouette
with transparency at top for irregular profile).
Each tree-quad: 5 m wide × 12 m tall crossed-plane pair.
Total count: ~180 billboard trees in a ring.
Rendered alpha-blended, no shadow. Fog heavily attenuates them.
```

## 9.3 Layer 3 — Far Horizon Cylindrical Billboard (150 m beyond fence)

A cylindrical mesh (radius = 280 m, height = 60 m, 128 segments, open top and bottom) centred at world origin. The cylinder's inner face is textured with `horizon_far.png` (a panoramic painted backdrop: dark treeline silhouette at the bottom, graduating to sky colour at the top).

```cpp
// Horizon cylinder generation:
int segments = 128;
float r = 280.0f, h = 60.0f;
for (int i = 0; i < segments; ++i) {
    float angle1 = (float)i     / segments * 2.0f * PI;
    float angle2 = (float)(i+1) / segments * 2.0f * PI;
    // 4 vertices: bottom-left, bottom-right, top-right, top-left of inner face
    // UV: U = i/segments to (i+1)/segments, V = 0 (bottom) to 1 (top)
}
// Rendered with alpha shader, fog factor = 1.0 at its distance → fully fog-blended
// No depth write. Always renders behind everything else.
// The horizon color of this texture matches the sky horizon keyframe color
// dynamically — use a uniform colorTint to tint the texture toward the
// current fogColor (multiply blend).
```

## 9.4 Ground Fog Layer

A thin fog band near the ground (Y = 0 to 2.0 m) is simulated by drawing a large low-height billboard quad at Y = 0.5 m, with a white-grey soft alpha gradient texture, alpha ~ 0.15 at night and 0.0 at noon. This visually anchors the far treeline to the ground.

---

# 10. Fractal Systems

## 10.1 L-System Trees — Poplar Style

**Placement:** 10 trees per side of Lagerstrasse (Z = -68 road), at X = -120, -107, -94, -81, -68, -55, -42, -29, -16, -3 on Z = -72 (north side) and Z = -64 (south side) = 20 trees total. Additional 6 trees near admin zone.

**Grammar (same as original, with 3D extension):**
```
Axiom: F
Rules: F → FF+[+F-F-F]-[-F+F+F]
Iterations: 4 (produces manageable mesh, ~400–600 cylinder segments per tree)
Angle: 22.5°
Azimuthal jitter: ±18° random rotation per branch fork (seeded per tree)
```

**Geometry per segment:**
```
Depth 0 (trunk):  cylinder r=0.25 m, length=1.8 m
Depth 1:          r=0.18 m, length=1.4 m
Depth 2:          r=0.12 m, length=1.1 m
Depth 3:          r=0.07 m, length=0.8 m
Depth 4 (tips):   r=0.04 m, length=0.5 m → leaf quad appended

Leaf quad (at tips only):
  Two crossed planes, each 0.8 m × 0.8 m, leaf_alpha.png
  Camera-facing (billboard: Y-axis constrained billboard, not full spherical)
  Rendered in alpha pass.
```

**Flyweight:** All 26 trees share `bark.png` and `leaf_alpha.png`. Each tree has a unique randomly-seeded geometry array built once at startup. Trunk VAO is shared only if two trees happen to share the same random seed (unlikely), otherwise each tree has its own VAO but shares texture.

## 10.2 Koch Snowflake Texture Generation

Generated at startup, written to a 64×64 RGBA texture (off-screen FBO or CPU buffer):

```cpp
// 3 iterations of Koch snowflake on CPU, rendered to pixel buffer
// Starting equilateral triangle edge length = 56 px
// Each recursive subdivision replaces middle third of edge with outward equilateral bump
// Fill algorithm: scanline fill of final polygon
// Write white (#FFFFFF) with anti-aliased edges to alpha channel
// Upload with glTexImage2D to fractal_snowflake.png slot
```

## 10.3 Snowflake Particle System

```cpp
struct Snowflake {
    glm::vec3 position;
    float speed;      // 0.3 – 1.2 m/s
    float drift;      // horizontal drift, varies: -0.2 to +0.2 m/s in X and Z
    float spin;       // rotation for billboard orientation, 0 – 2π
    float spinRate;   // rad/s
    float size;       // 0.05 – 0.25 m (varies per flake)
    float alpha;      // 0 – 1 (fade based on time)
};

// 600 snowflakes, uniformly distributed over X=[-130,130], Z=[-100,100], Y=[0,30]
// Each frame:
//   position.y -= speed * dt
//   position.x += drift.x * dt
//   position.z += drift.z * dt
//   spin += spinRate * dt
//   if position.y < 0.0: reset to random above scene
// Alpha envelope:
//   fade_in  = smoothstep(19.0, 21.0, timeOfDay)
//   fade_out = 1.0 - smoothstep(4.5, 6.5, timeOfDay)
//   flake.alpha = min(fade_in, fade_out)
// Render as billboarded quads, Z-rotation = spin, unlit shader
// Snow settles effect: snowflakes near ground (Y < 0.3) become 50% transparent
```

## 10.4 Chimney Smoke Particles

```cpp
struct SmokeParticle {
    glm::vec3 position;  // starts at chimney top (X=-125, Y=12, Z=-32)
    glm::vec3 velocity;  // upward + small random XZ drift
    float lifetime;      // 0.0 – 4.0 s
    float age;
    float size;          // grows from 0.3 to 1.5 m as it rises
    float alpha;         // fades out as age increases
};

// 80 smoke particles. Spawn rate: 4/s.
// velocity: vec3(rand(-0.2,0.2), 0.8 + rand(0,0.4), rand(-0.2,0.2))
// Color: dark grey (#444444) at base → lighter grey (#AAAAAA) → white as it disperses
// Render: camera-facing billboard quads, alpha-blended, no depth write
// Size = lerp(0.3, 1.8, age/lifetime)
// Alpha = (1 - age/lifetime) * 0.6
// Active day and night (crematorium runs continuously)
```

---

# 11. Interior Rendering System

Interiors are rendered using a **portal/proximity system** to avoid the cost of always rendering all 28 interiors:

```cpp
// For each barrack block, compute distance from camera to block centre.
// If distance < 25.0 m: render full interior geometry (drawInterior = true)
// If distance < 50.0 m: render window glow quads on exterior face only
// If distance >= 50.0 m: skip interior entirely

// Window glow (exterior face, distance 25–50 m):
//   A flat emissive quad placed just inside the window opening, facing outward.
//   Color: warm yellow (#FFEE88) at low intensity. Rendered with unlit shader.
//   This simulates interior light visible from outside without rendering the
//   full interior mesh.

// Interior/exterior separation:
//   The interior back-face of the exterior wall is NOT rendered (exterior walls
//   use back-face culling). Interior walls are separate geometry rendered only
//   when drawInterior = true.
//   Interior ceiling: rendered only when camera Y < 4.5 m (inside ground floor).
```

**Window glass rendering:**
- Window glazing quads are rendered in the alpha pass with `glass_alpha.png`.
- From outside: slight grey tint, moderate transparency (alpha ~ 0.6), specular highlight.
- From inside: same geometry, no back-face culling for glazing quads so they're visible from both sides.
- At night with interior lights active: apply additive emissive component to the glass quad tinted warm yellow — gives the appearance of light glowing through glass.

---

# 12. Camera System

## 12.1 Controller

```cpp
class Camera {
    glm::vec3 position = glm::vec3(+135.0f, 1.7f, 0.0f); // start at gate, eye height
    glm::vec3 front    = glm::vec3(-1.0f, 0.0f, 0.0f);    // facing into camp (west)
    glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f);
    float yaw = 180.0f, pitch = 0.0f;
    float movementSpeed     = 7.0f;   // m/s walking
    float sprintMultiplier  = 3.5f;
    float mouseSensitivity  = 0.08f;
    float fov = 65.0f;
};
```

## 12.2 Boundary Clamp

```cpp
// After processing movement, hard-clamp to safe bounds:
position.x = clamp(position.x, -138.0f, +138.0f);
position.z = clamp(position.z, -98.0f,  +98.0f);
position.y = clamp(position.y,   1.5f,   12.0f);
```

## 12.3 Controls

| Input | Action |
|---|---|
| W / A / S / D | Move forward / left / backward / right |
| Mouse | Look (yaw + pitch, pitch clamped ±88°) |
| Left Shift | Sprint (3.5× speed) |
| Space | Move up |
| C | Move down |
| T | Cycle time speed: paused → 1× → 8× → 30× |
| F | Toggle wireframe |
| H | Toggle HUD overlay |
| ESC | Release mouse / quit |
| Left Click | Capture mouse |

## 12.4 HUD Overlay

Render using a simple bitmap font or a single-channel texture atlas for ASCII characters (no external font library required — include a 128-character 8×8 pixel font as a 1024×8 texture and render each character as a small quad).

Display:
```
[top-left]  Time: 14:32  |  Clouds: Clear  |  Temp: [zone name]
```
Zone name is determined by current camera X/Z position, mapped to zone names
(e.g. "Main Barracks", "Entrance Gate", "Crematorium", "Block 11 Courtyard", etc.)

---

# 13. Material Properties

| Surface | Ambient Str. | Diffuse Str. | Specular Str. | Shininess |
|---|---|---|---|---|
| Brick walls (standard) | 0.12 | 0.82 | 0.04 | 4 |
| Brick (dark, Block 11) | 0.06 | 0.65 | 0.03 | 4 |
| Metal fence / gate iron | 0.08 | 0.45 | 0.95 | 96 |
| Metal lamp / grey steel | 0.10 | 0.50 | 0.80 | 64 |
| Wood planks (towers) | 0.18 | 0.78 | 0.08 | 8 |
| Wood (dark, doors) | 0.15 | 0.70 | 0.05 | 6 |
| Ground gravel | 0.22 | 0.92 | 0.02 | 2 |
| Road dirt | 0.20 | 0.88 | 0.01 | 2 |
| Roof tiles | 0.13 | 0.72 | 0.06 | 8 |
| Concrete posts | 0.18 | 0.82 | 0.06 | 6 |
| Glass (windows) | 0.05 | 0.30 | 0.95 | 256 |
| Stone floor | 0.15 | 0.80 | 0.05 | 16 |
| Cobblestone | 0.18 | 0.78 | 0.04 | 8 |

---

# 14. Flyweight Pattern — Implementation

```cpp
// Flyweight.h
class MeshFlyweight {
public:
    GLuint VAO, VBO, EBO;
    GLuint textureID;         // primary diffuse
    GLuint specularTexID;     // optional
    int    indexCount;

    void draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    }
};

class FlyweightFactory {
    std::unordered_map<std::string, std::unique_ptr<MeshFlyweight>> pool;
public:
    MeshFlyweight* get(const std::string& key);
    void           build(const std::string& key, /* geometry builder fn */);
};

struct InstanceTransform {
    glm::mat4 model;
    glm::vec3 colorTint = glm::vec3(1.0f);
};

// Keys in the pool:
//  "barrack_standard"  → 28 instances (barracks A/C/D type)
//  "barrack_dark"      → 2 instances (blocks 10, 11)
//  "fence_post"        → ~448 instances (inner + outer fence posts)
//  "guard_tower"       → 12 instances
//  "street_lamp"       → ~85 instances
//  "road_section"      → ~60 instances (road quad tiles)
//  "tree_trunk_depth0" → shared bark texture (not full mesh)
```

---

# 15. Render Pipeline

## 15.1 Pass Order Each Frame

```
Pass 1 — Shadow depth:
  glBindFramebuffer(shadow FBO), glViewport(0,0,4096,4096)
  Render all opaque geometry with shadowmap.vert/frag (depth only)
  No skybox, no particles, no transparent geometry

Pass 2 — Skybox:
  glDepthMask(false), glDisable(GL_DEPTH_TEST)
  Draw sky hemisphere (skybox.vert/frag), no depth write
  glDepthMask(true),  glEnable(GL_DEPTH_TEST)

Pass 3 — Stars (if night):
  Draw star point sprites, additive blend, no depth write

Pass 4 — Sun / Moon spheres:
  Unlit shader, depth test on but no depth write

Pass 5 — Opaque geometry (main Phong pass):
  Bind shadow map texture to unit 4
  Update all light uniforms (cull distant lamps)
  Render: ground, roads, all barrack exteriors, fence posts,
          towers, gate, crematorium, admin buildings
  Interiors for blocks within 25 m of camera also drawn here
  Depth write on

Pass 6 — Alpha / transparent geometry (sorted back-to-front):
  glEnable(GL_BLEND), glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
  Render: window glass, fence wire, tree leaves, horizon layer 2 billboard trees
  Horizon layer 3 cylindrical billboard last (furthest)
  Ground fog quad
  glDisable(GL_BLEND)

Pass 7 — Particles:
  Smoke: alpha blend
  Snowflakes: alpha blend, no depth write
  Both: particle.vert/frag
```

---

# 16. File Structure

```
auschwitz_tour/
├── README.md
├── extern/
│   ├── glad/glad.h + glad.c
│   └── stb/stb_image.h
├── src/
│   ├── main.cpp
│   ├── Camera.h/.cpp
│   ├── Shader.h/.cpp
│   ├── Texture.h/.cpp
│   ├── Mesh.h/.cpp
│   ├── Flyweight.h/.cpp
│   ├── Scene.h/.cpp
│   ├── DayNightCycle.h/.cpp
│   ├── LSystemTree.h/.cpp
│   ├── BezierCurve.h/.cpp
│   ├── ParticleSystem.h/.cpp
│   ├── ShadowMap.h/.cpp
│   ├── LightCuller.h/.cpp        ← NEW: frustum-cull distant lamps
│   ├── InteriorRenderer.h/.cpp   ← NEW: proximity-gated interior pass
│   ├── HorizonSystem.h/.cpp      ← NEW: 3-layer horizon
│   ├── HUD.h/.cpp                ← NEW: bitmap font overlay
│   ├── zones/
│   │   ├── EntranceZone.cpp
│   │   ├── BarrackGrid.cpp
│   │   ├── BarrackInteriorA.cpp  ← Type A sleeping quarters
│   │   ├── BarrackInteriorB.cpp  ← Type B washroom
│   │   ├── BarrackInteriorC.cpp  ← Type C Block 11
│   │   ├── Block11Zone.cpp
│   │   ├── AdminZone.cpp
│   │   ├── CrematoryZone.cpp
│   │   ├── FenceSystem.cpp
│   │   └── Environment.cpp
│   └── primitives/
│       ├── Cube.h/.cpp
│       ├── Cylinder.h/.cpp
│       ├── Sphere.h/.cpp
│       ├── Plane.h/.cpp
│       ├── BezierTube.h/.cpp     ← NEW: extrude cross-section along Bezier
│       └── RuledSurface.h/.cpp   ← NEW: ruled surface mesh generator
├── shaders/
│   ├── phong.vert + phong.frag
│   ├── unlit.vert + unlit.frag
│   ├── particle.vert + particle.frag
│   ├── skybox.vert + skybox.frag
│   ├── alpha.vert + alpha.frag
│   ├── shadowmap.vert + shadowmap.frag
│   └── interior.vert + interior.frag
└── textures/
    └── [all files from Section 8.1]
```

---

# 17. Implementation Checklist — Phase by Phase

## Phase 1 — Foundation
- [ ] GLFW window + GLAD init, 1280×720, 4x MSAA
- [ ] Shader class: load, compile, link, uniform setters (vec3, mat4, int, float arrays)
- [ ] Camera: WASD + mouse, boundary clamp, sprint, Y movement
- [ ] Cube, Cylinder, Sphere, Plane primitives with correct normals and UVs
- [ ] Single Phong-lit textured cube confirmed working
- [ ] BezierTube primitive: extrude circle cross-section along N-point Bezier — confirm with test arch

## Phase 2 — Scene Scaffolding
- [ ] Ground plane (270×210 m) with gravel.png, anisotropic filtering
- [ ] Road network quads (per Section 2.2), no overlap with barrack footprints verified
- [ ] Flyweight factory: barrack_standard mesh built, 28 instances placed at correct grid positions
- [ ] Basic directional Phong (sun only), confirm no shadow yet
- [ ] Perimeter inner + outer fence posts (Flyweight, ~448 posts)
- [ ] Fence wire strands via Bezier droop (5 per bay, all perimeter sides)

## Phase 3 — Key Landmarks & Exterior Detail
- [ ] Entrance gate: gatehouse (two wings + tunnel), gate arch (Bezier), gate leaves, text billboard, flagpoles
- [ ] Guard towers ×12: all sub-components (legs, cross-bracing, platform, railing, cabin, roof, ladder, searchlight housing)
- [ ] Street lamps ×85: Flyweight, Bezier arm, housing cone, emissive bulb sphere
- [ ] Block 11 zone: dark texture, barred windows, courtyard enclosure walls, Death Wall
- [ ] Crematorium: main building, chimney (cylinder), ruled-surface roof, furnace annex, gallows
- [ ] Admin zone: 4 buildings (commandant, SS HQ, kitchen, reception) with distinct roof geometry

## Phase 4 — Lighting & Shadows
- [ ] Multi-light Phong shader compiled (see Section 4.2 — full shader code)
- [ ] LightCuller: frustum cull distant spot lamps, pass ≤24 lamps + 12 towers per frame
- [ ] Shadow map FBO (4096×4096), PCF 5×5, slope-scale bias
- [ ] Material uniforms per surface type (Section 13)
- [ ] Street lamp spotlights: downward cone, activate at night, emissive bulb
- [ ] Tower searchlights: sweep behaviour (Section 5.2), activate at night
- [ ] Interior point lights: stove (orange), ceiling bulbs (warm white)
- [ ] Death Wall near-black ambient area
- [ ] Distance fog (Section 4.2 — fog mix in phong.frag)

## Phase 5 — Day/Night Cycle
- [ ] DayNightCycle class: timeOfDay float, update(dt), speed multiplier
- [ ] Sky hemisphere with keyframe gradient (Section 6.1 shader + CPU table)
- [ ] Sun orbit + emissive sphere, moon orbit + emissive sphere (Section 6.2)
- [ ] Stars: 800 point sprites, fade in/out with time (Section 6.3)
- [ ] Global ambient transition 0.04–0.30 smooth curve
- [ ] Fog colour follows sky horizon colour keyframe
- [ ] Lamp/searchlight activation thresholds with smooth fade
- [ ] Keyboard time-speed cycle (T key)

## Phase 6 — Fractals
- [ ] L-system tree generator: grammar, 3D turtle, cylinder segments, leaf billboards (Section 10.1)
- [ ] 26 trees placed (20 Lagerstrasse + 6 admin)
- [ ] Koch snowflake texture: CPU generation at startup → upload to GL texture (Section 10.2)
- [ ] Snowflake particle system: 600 particles, drift, spin, fade envelope (Section 10.3)
- [ ] Chimney smoke system: 80 particles, size growth, colour fade (Section 10.4)

## Phase 7 — Interiors
- [ ] Interior Type A (sleeping quarters): bunk beds, columns, stove, ceiling bulbs, table (Section 3.2)
- [ ] Interior Type B (washroom): trough, taps, bench (Section 3.2)
- [ ] Interior Type C (Block 11): cells, corridor, standing cells (Section 3.2)
- [ ] Proximity gate: draw interior only within 25 m (Section 11)
- [ ] Window glow quads: emissive, visible from outside 25–50 m (Section 11)
- [ ] Window glass alpha quads: exterior + interior visible (Section 11)
- [ ] Light-through-window shaft (Block 11, day only): narrow spotlight

## Phase 8 — Horizon & Atmosphere
- [ ] Ground extension plane beyond fence (Section 9.1)
- [ ] Mid-ground billboard treeline ring ×180 (Section 9.2), alpha-blended
- [ ] Horizon cylinder (r=280 m, 128 segs), tinted by fogColor uniform (Section 9.3)
- [ ] Ground fog billboard quad (Section 9.4)
- [ ] Catmull-Rom road edge splines at junctions (Section 7.4)

## Phase 9 — Polish & Performance
- [ ] HUD bitmap font overlay: time, zone name (Section 12.4)
- [ ] Verify no road/building footprint overlap (Section 2.2)
- [ ] Verify all Flyweight pools reduce unique VAOs to < 50
- [ ] Profile: confirm ≥30 FPS on target hardware
- [ ] Appellplatz: verify large open gravel area, lamp perimeter, open sky
- [ ] Final camera start position: X=+135, Z=0, facing west into camp

---

# 18. Academic Feature Mapping

| Academic Topic | Demonstrated By | File |
|---|---|---|
| 3D Modelling | Barracks, towers (multi-part), gate, crematorium, lamps, bunks, gallows | zones/, primitives/ |
| Transformations | All instances via glm model matrices; Flyweight extrinsic transforms | Flyweight.cpp |
| Bezier Curves | Gate arch extrusion, lamp arm, fence wire droop | BezierCurve.cpp, BezierTube.cpp |
| Spline Curves | Catmull-Rom road junction edges | BezierCurve.cpp |
| Ruled Surface | Crematorium roof (two parallel Bezier curves) | RuledSurface.cpp |
| Texture Mapping | All 26 texture assets, explicit UV coords | Texture.cpp, all zones |
| Mipmapping | glGenerateMipmap, GL_LINEAR_MIPMAP_LINEAR on all wall/ground textures | Texture.cpp |
| Wrapping | GL_REPEAT on walls/ground; GL_CLAMP_TO_EDGE on sprites/glass/text | Texture.cpp |
| Anisotropic | Ground, roads, brick walls at grazing angles | Texture.cpp |
| Vertex Shader | MVP, normal matrix, light-space transform | phong.vert |
| Fragment Shader | Full Phong loop, fog, shadow PCF, texture sample | phong.frag |
| Phong Shading | Per-fragment ambient+diffuse+specular | phong.frag |
| Point Lights | Stove (orange), ceiling bulbs (warm) per barrack; admin/crema lights | BarrackInteriorA.cpp |
| Directional Light | Sun (day) + moon (night) as direction vectors | DayNightCycle.cpp |
| Spotlights | 12 tower searchlights (sweeping) + 85 street lamps (downward) + gate spots | FenceSystem.cpp, phong.frag |
| Ambient Illumination | Global ambient 0.04–0.30 day/night | DayNightCycle.cpp |
| Diffuse Illumination | Lambertian on all surfaces | phong.frag |
| Specular Illumination | High specular on metal fence, gate iron, glass, lamp housings | phong.frag, material structs |
| Fractal — Trees | L-system grammar, 3D turtle, cylinder mesh, billboard leaves | LSystemTree.cpp |
| Fractal — Snow | Koch snowflake CPU generation, particle system | ParticleSystem.cpp |
| Flyweight | Factory pool: barracks, posts, towers, lamps, road sections | Flyweight.cpp |
| LOD Horizon | 3-layer parallax: ground ext. + treeline billboards + cylinder backdrop | HorizonSystem.cpp |
| Shadow Mapping | 4096 depth FBO, orthographic sun projection, PCF 5×5 | ShadowMap.cpp |

---

# 19. Non-Negotiables & Constraints

- OpenGL Core Profile 3.3 ONLY. No `glBegin`/`glEnd`, no `glMatrixMode`.
- All geometry via VAO/VBO/EBO. All shaders GLSL 330 core.
- GLM for all math. stb_image for all texture loading.
- No external font or UI libraries — bitmap font via texture atlas only.
- No game mechanics, no objectives, no health/score UI.
- Tone: educational, solemn, historically respectful.
- Camera starts at the east gate, facing west into the camp.
- Roads must not overlap barrack footprints — verify with bounds checks during placement.
- All textures are placeholder-named; generate or source appropriate PNGs. Procedural solid-colour textures are acceptable for a working submission (e.g., a flat red PNG for brick_red.png) as long as the UV mapping, mipmapping, and shader integration are correctly demonstrated.

---

*This document is the complete enhanced implementation prompt for the Auschwitz I 3D educational reconstruction. All geometry, coordinates, measurements, lighting specifications, interior details, shader code, and render pipeline are fully defined above. Implement each phase in sequence following the Phase checklist in Section 17.*