---
name: ue-level-audit-design
description: "Use when auditing, designing, expanding, or balancing Unreal Engine levels for modularity, spatial density, horror atmosphere, lighting complexity, NavMesh coverage, and performance. Also use when running level inspection scripts (ue_level_analyzer.py, analyze_level1.py), comparing map file sizes or actor metrics, converting level themes (office to industrial/concrete), or verifying level readiness before release. For streaming and World Partition, see ue-world-level-streaming; for dynamic lighting/MIDs, see ue-materials-rendering; for AI pathing, see ue-ai-navigation."
metadata:
  version: "1.0.0"
  engine: "5.8"
---

# UE Level Architectural Audit & Design

Target engine: **UE 5.8**.

This skill provides the end-to-end procedure for auditing and constructing levels in Unreal Engine, specifically tuned for modular horror environments (Backrooms). It covers spatial density analysis, modular snapping, industrial lighting calibration, NavMesh verification, and in-editor Python automation.

## Context
Read `.agents/ue-project-context.md` for map names, sound mix classes, physical materials, and renderer scalability settings.

| Task | Section |
|---|---|
| Measuring level density & actor deficits | [Level Density & Audit Heuristics](#level-density--audit-heuristics) |
| Running Python level inspection | [Python Automation & In-Editor Analysis](#python-automation--in-editor-analysis) |
| Modular geometry & layout rules | [Modular Spatial Construction](#modular-spatial-construction) |
| Lighting & horror atmosphere calibration | [Lighting & Post-Process Standards](#lighting--post-process-standards) |
| NavMesh & AI validation | [Navigation & AI Readiness](#navigation--ai-readiness) |
| Audio ambience & surface physics | [Audio & Physical Material Integration](#audio--physical-material-integration) |
| Final acceptance checklist | [Level Verification Checklist](#level-verification-checklist) |

## Level Density & Audit Heuristics
When evaluating whether a level is production-ready, use the **Reference Ratio Method**:
- **Baseline Comparison:** Compare target level `.umap` file size against a known polished reference (e.g., Level 0: 7.77 MB, ~1000 actors). If a map is <50% of the reference size, it suffers from severe spatial/prop/lighting underdevelopment.
- **Actor Density Benchmarks (per playable zone):**
  - Total Actors: 850–950
  - Static Mesh Actors: 500–600
  - Lights: 180–200 fixtures
  - Environmental Storytelling Props: 80–120 props (pipes, boilers, panels, barrels)
  - Ambient Sound Emitters: 40–60 spatialized emitters

## Python Automation & In-Editor Analysis
1. **Standalone Metadata Scan (`analyze_level1.py`):**
   Run via terminal to inspect file sizes, package headers, and output execution instructions:
   ```powershell
   python analyze_level1.py
   ```
2. **In-Editor Python Inspection (`ue_level_analyzer.py`):**
   In Unreal Editor (`Window -> Developer Tools -> Output Log -> Python`):
   ```python
   exec(open(r'c:\Users\Administrator\Documents\Unreal Projects\Backrooms 5.8\ue_level_analyzer.py').read())
   ```
   Inspect generated `LEVEL1_ANALYSIS_DATA.json` for actor counts by class, light intensities/colors, bounds `[min_xyz, max_xyz]`, and missing volume warnings.

## Modular Spatial Construction
- **Grid Snapping:** Strict 100cm / 400cm grid snapping for modular walls, floors, and ceilings (`SM_Backrooms_Floor_1M`, `SM_Backrooms_Wall_1M`, `SM_Backrooms_Ceiling_1M`).
- **Corridor Topology Ratio:**
  - 40% Straight corridors (`SM_Corridor_Straight_400`)
  - 35% L-turns / Corners (`SM_Corridor_Corner_400`) to break sightlines (<8m visible range)
  - 25% Junctions / Crosses (`SM_Corridor_TJunction_400`, `SM_Corridor_Cross_400`)
- **Room Diversity Taxonomy:**
  - *Utility / Storage Closet:* 2m × 2m (claustrophobic, single overhead bulb)
  - *Maintenance Office:* 4m × 5m (desks, monitors, files)
  - *Electrical Substation:* 5m × 5m (switchboards, transformer drone)
  - *Pump / Valve Station:* 6m × 10m (water puddles, piping networks)
  - *Boiler Room / Hub:* 8m × 8m to 12m × 12m (central landmark, multi-pipe junctions)

## Lighting & Post-Process Standards
- **Level Thematic Distinction:**
  - **Level 0 (Liminal Office):** Warm 2700K (`#FFEFB8`), 300 cd, 950 cm radius PointLights; EV100 `[0.0, 5.0]`, bias `+0.75`.
  - **Level 1 (Industrial Basement):** Cool 4500K (`#FFF4E8`), 200–250 cd, 700–800 cm radius SpotLights (60° cone); EV100 `[-1.0, 4.0]`, bias `+0.25`.
- **Flicker State Distribution:**
  - 70% Stable / steady fixtures
  - 20% Flickering / Erratic (`BP_Level1_FlickeringLight`, zero-tick curve/timer)
  - 10% Dead / Broken (dark zones forcing flashlight utilization)
- **Light Complexity:** Verify in editor with `Alt + 0`. No more than 3–4 overlapping dynamic attenuation spheres on any surface.

## Navigation & AI Readiness
- Every level must contain a `NavMeshBoundsVolume` covering the entire walkable floor.
- Navigation settings matching Backrooms character/entity capsules:
  - Agent Radius: `42.0`
  - Agent Height: `192.0`
  - Cell Size: `19.0`
- Press **P** in viewport to confirm 100% continuous green overlay. Zero isolated red pockets or broken door thresholds.

## Audio & Physical Material Integration
- **Sound Routing:** All room tone, hums, drips, and steam must route through `SC_Ambience` (volume ≤0.5) to prevent masking monster cues in `SC_Entity`.
- **Surface Materials:**
  - Carpet: `MI_Backrooms_Carpet` assigned `PM_Carpet` (`SurfaceType1`).
  - Concrete/Tile: `MI_Level2_Floor_Tile` assigned `PM_Concrete` (`SurfaceType2`).
  - Character footstep trace in `ABackroomsCharacter::TriggerFootstep()` detects `PhysicalMaterial` to swap audio sample sets.

## Level Verification Checklist
Before approving a level as complete, verify:
- [ ] Actor count within 850–950 benchmark
- [ ] 180–200 lights placed with proper attenuation radius (<800cm)
- [ ] 100% NavMesh coverage verified via 'P' debug mode
- [ ] Physical materials mapped on 100% of walkable surfaces
- [ ] 40/35/25 corridor proportion maintained
- [ ] Ambient emitters routed to `SC_Ambience` at ≤0.5 volume
- [ ] 60+ FPS at Low scalability settings
