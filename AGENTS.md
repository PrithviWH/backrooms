# Backrooms 5.8 — Workspace Rules & Guardrails

These rules apply across the entire project for all level design, C++, and Blueprint changes.

## 1. Level Design & Navigation Invariants
- **NavMesh Bounds Mandatory:** Every playable map (`.umap`) MUST contain a `NavMeshBoundsVolume` configured with Agent Radius `42.0`, Agent Height `192.0`, and Cell Size `19.0`. NavMesh must be rebuilt and verified (green overlay with viewport shortcut `P`) before committing map changes.
- **Modular Grid Snapping:** Corridor and room geometry must adhere to 100cm / 400cm grid snapping (`SM_Backrooms_Floor_1M`, `SM_Backrooms_Wall_1M`, `SM_Corridor_Straight_400`, etc.).
- **Physical Materials on Floors:** Every floor material must have an assigned `PhysicalMaterial` (`PM_Carpet` / `SurfaceType1` for Level 0, `PM_Concrete` / `SurfaceType2` for Level 1) to ensure character and entity ground line-traces trigger correct footstep audio.

## 2. Lighting & Performance Guardrails
- **Light Complexity:** Keep dynamic light attenuation spheres bounded (700–800cm max for Level 1 SpotLights). Avoid overlapping attenuation radii that trigger shader complexity spikes; verify with Light Complexity view mode (`Alt + 0`).
- **Low-Tier Target:** All levels must maintain 60+ FPS on Low scalability settings (<16ms frame time, draw calls <1500).

## 3. Audio Class Routing
- All ambient environment emitters (drones, pipes, steam, fluorescent buzz) MUST route through `SC_Ambience` and must not exceed volume 0.5, ensuring monster cues in `SC_Entity` remain fully audible.

## 4. Entity Horror Presentation & Sight-Gated Audio Invariants
- **Visual Obscuration & Lighting-Dependent Silhouette:** The entity must never appear brightly lit or reveal raw default mannequin textures, armor panels, decals, seams, or cyan/blue chest logos. It uses `M_EntityShadowVoid`:
  - When standing beneath an active ceiling light, it renders as a dark obsidian humanoid silhouette (`BaseColor ~ (0.018, 0.015, 0.012)`, `Roughness = 0.92`, `Specular = 0.08`) catching realistic contour lighting against yellow wallpaper.
  - When the light directly above is turned off/blacked out or in unlit areas, the entity dynamically becomes **100% pitch-black** (`BaseColor = (0, 0, 0)`, `Roughness = 1.0`, `Specular = 0.0`, `Metallic = 0.0`, zero emissive), blending seamlessly into darkness with zero reflective highlights or visible facial/body details.
- **Distant Stalker Psychological Horror Behavior:**
  - The entity operates primarily in `EBackroomsAIState::DistantStalker`, standing completely motionless and silent down far corridors, around distant corners, or near intersections ($18\text{--}34\text{ meters}$ away) creating the uncanny dread of being watched.
  - **Precision Ground Trace & Anti-Burial Invariant:**
    - Raw NavMesh projection points ($Z_{\text{nav}}$) lie on the floor surface; teleporting an `ACharacter` directly to $Z_{\text{nav}}$ places its capsule center at the ground, burying 50% of its body.
    - Every vantage point MUST be validated with a downward raycast against `ECC_WorldStatic`, setting the spawn location to $Z_{\text{impact}} + \text{CapsuleHalfHeight} + 1.5\text{f}$.
    - Skeletal mesh relative Z must remain strictly `-CapsuleHalfHeight` (`-96.0f`), and `CharacterMovementComponent->FindFloor()` and `AdjustFloorHeight()` must be called upon arrival to guarantee flush floor alignment without penetration.
    - Floor candidate points must verify walkable surface normal ($Z \ge 0.65$), capsule obstacle clearance, and ceiling clearance.
  - **Encounter Lifecycle & Timer Reliability:**
    - Encounters must loop indefinitely across the entire exploration session without permanently stalling or leaving the entity vanished.
    - **Appearance interval:** $18\text{--}35\text{s}$ after an encounter concludes.
    - **Minimum visible duration:** $2.0\text{s}$ before look-away vanish can occur, ensuring the player has time to register the sighting.
    - **Maximum visible duration:** $15.0\text{s}$ timeout if the player stares continuously without looking away, dissolving into darkness to preserve suspense.
    - **Gaze Detection & Reset:** If the player glances away and then looks back at the entity, `StalkerLookAwayDuration` resets to $0.0\text{s}$ immediately so minor head movement does not trigger an instant vanish.
    - **Failed Spawn Retry:** If no valid vantage point is found (e.g. player is facing a dead-end wall), the system retries automatically in $2\text{--}4\text{s}$ rather than abandoning the encounter loop.
    - **Unseen Relocation:** If the entity is never observed for $30.0\text{s}$ (player walked in the opposite direction), it relocates ahead of the player's path.
  - **Psychological Disappearance:** Once the player notices and looks directly at the entity ($\ge 0.6\text{s}$), if the player looks away or ducks behind a wall ($\ge 0.5\text{s}$ after $\ge 2.0\text{s}$ visible duration), the entity instantly vanishes into the shadows and relocates to another distant corridor vantage point.
  - **Proximity Safeguard:** If the player sprints directly towards the entity within close range ($< 8.5\text{m}$), the entity vanishes into darkness before they can close distance.
- **Sight-Gated Panic Audio:** Scare string stingers and visual panic audio MUST strictly require line-of-sight from the player's camera:
  $$\vec{F}_{\text{camera}} \cdot \frac{\vec{P}_{\text{entity}} - \vec{P}_{\text{camera}}}{\|\vec{P}_{\text{entity}} - \vec{P}_{\text{camera}}\|} > 0.35$$
  combined with an unobstructed line-trace against `ECC_Visibility` and `!entity->IsHidden()`. Never trigger sight stingers, breath giveaways, or panic audio when the entity is behind the player, obscured by walls, or vanished.
- **Aggression & Melee Range:** In active chase, the entity pursues the player with locomotion animations and triggers its attack/jumpscare sequence upon reaching melee distance (~140cm), never halting passively in front of the player.

## 5. Player Controls & Input Sensitivity
- **First-Person Sensitivity Default:** Enhanced input mouse look sensitivity MUST default to `0.20f` (not 1.0f) in `ABackroomsCharacter` and `UBackroomsSaveGame` to ensure smooth, controlled camera movement.
- **Save Game Migration:** Any settings load routine must verify and migrate legacy `1.0f` values down to `0.20f` so existing saves do not retain high twitch sensitivity.

## 6. Atmosphere & Dynamic Spatial Audio
- **Continuous Dread Layering:** Every playable level must have an active `ABackroomsAtmosphereManager` providing a continuous background psychological drone (volume ~0.32) and sub-bass room tone (volume ~0.20).
- **Audio Class Routing & Volume Cap:** All ambient and environmental audio emitters MUST route through `SC_Ambience` and strictly remain $\le 0.5$ volume, reserving dynamic headroom for monster audio in `SC_Entity`.
- **Geometry-Aware 3D Horror Cues:** Dynamic exploration cues (distant door slams, wall knocks, heavy impacts, whispers) must be spatialized in 3D, triggered contextually by level geometry (dead ends, blind corners, long halls), and throttled with suspense pacing intervals (20–45s).
