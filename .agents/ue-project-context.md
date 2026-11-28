# Backrooms — UE Project Context

**Generated:** 2026-11-27  
**Engine version:** Unreal Engine 5.8 (Launcher / Binary Build)  
**Project file:** `Backrooms.uproject`  

---

## Levels & Environments
| Map File | Level Name | Canonical Theme | Primary Floor / Wall Materials | Lighting Style | Target Actor Count |
|---|---|---|---|---|---|
| `L_Backrooms_Main.umap` | Level 0 | Infinite Office Carpet Maze | `MI_Backrooms_Carpet` / `MI_Backrooms_Wallpaper` | Warm 2700K PointLights (300 cd, 950cm radius) | ~1000 |
| `backroomsLevel2.umap` | Level 1 | Industrial Maintenance / Basement | `MI_Level2_Floor_Tile` / `MI_Level2_Wall_OffWhite` & `MI_Level2_Wall_Crimson` | Cool 4500K SpotLights (200-250 cd, 700cm radius) | 850–950 |

## Modules
**Primary game module:** `Backrooms`

| Module | Host type | Public deps | Private deps | Notes |
|---|---|---|---|---|
| `Backrooms` | Runtime | `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `AIModule`, `NavigationSystem`, `GameplayTasks`, `Slate`, `SlateCore` | *none* | Main gameplay and core C++ module (`ABackroomsGameModeBase`, `ABackroomsPlayerController`, `ABackroomsAIController`, `ABackroomsPauseMenu`, `ABackroomsSettingsMenu`, `UBackroomsSaveGame`, `UBackroomsGameInstance`, `UBackroomsFunctionLibrary`) |

## Plugins
| Plugin | Maturity | Used for |
|---|---|---|
| `ModelingToolsEditorMode` | Stable | Editor-side procedural & architectural mesh editing |
| `GameplayStateTree` | Stable | AI logic, behavioral transitions, and state tree execution |
| `AllToolsets` | Editor | Unreal Model Context Protocol toolsets registration |
| `ModelContextProtocol` | Editor | MCP server exposing engine automation tools on port 8000 |

## Coding conventions
**Prefixes:** Standard Unreal (`A` for Actors, `U` for UObjects/Components, `F` for structs, `E` for enums, `I` for interfaces, `BP_` for Blueprints, `WBP_` for Widget Blueprints, `M_`/`MI_` for Materials/Instances).  
**Object references:** `TObjectPtr<T>` in reflected `UPROPERTY()`, `TWeakObjectPtr<T>` for non-owning references.  
**API macro style:** `BACKROOMS_API` per reflected class declaration.  
**Header layout:** Flat module layout in `Source/Backrooms/`.

## Gameplay framework
| Role | Class |
|---|---|
| GameMode | `BP_BackroomsGameMode` (derives from native `ABackroomsGameModeBase`) |
| GameInstance | `BackroomsGameInstance` (`UBackroomsGameInstance`, persistent inventory, level tag progression, safe `SavePlayerState`/`LoadPlayerState` blueprint compatibility stubs) |
| PlayerController | `BP_BackroomsPlayerController` (derives from native `ABackroomsPlayerController`, `IA_Pause` handling, pause menu toggling, input mode & mouse cursor control) |
| Pawn / Character | `BP_BackroomsCharacter` (derives from native `ABackroomsCharacter`, Enhanced Input `IMC_Player`, ungated free sprint 300/600 cm/s with dynamic FOV 90°–98°, footstep audio system, mouse sensitivity & invert Y integration, native first-person camera collision prevention) |
| HUD / UI | `WBP_BackroomsHUD` (minimalist liminal horror UI with flashlight status, level intro, interaction prompts; stamina bar removed) |
| Pause Menu | `WBP_PauseMenu` (derives from native `UBackroomsPauseMenu`, 70% dark horror overlay, Resume, Settings, Quit) |
| Settings Menu | `WBP_SettingsMenu` (derives from native `UBackroomsSettingsMenu`, 85% dark horror overlay, Audio/Graphics/Controls tabs, live 0.5s FPS counter) |
| Save Game | `BackroomsSaveGame` (`UBackroomsSaveGame`, persistent audio volumes, scalability presets, resolution scale, mouse sensitivity, invert Y) |
| Entity / Monster | `BP_BackroomsEntity` (derives from native `ABackroomsEntity`, spatialized 3D footstep audio, state-driven cadence/volume, ambient breathing, dynamic proximity ceiling light distortion & blackout) |
| AI Controller | `BP_BackroomsAIController` (derives from native `ABackroomsAIController`) |
| Function Library | `BackroomsFunctionLibrary` (`UBackroomsFunctionLibrary` — camera fade, math, utilities) |
| Ambient Manager | `BP_Backrooms_AmbientManager` |

## Horror Proximity & Lighting Distortion Systems
* **Proximity Heartbeat & Panic System (`ABackroomsCharacter`):**
  * Internal 2D stereo `HeartbeatAudioComponent` (non-spatialized direct to player's ears) routed through `SC_SFX`.
  * Dynamic proximity tracking to nearest `ABackroomsEntity` within `MaxHeartbeatDistance = 2500.0f` ($25\text{ m}$) down to `MinHeartbeatDistance = 400.0f` ($4\text{ m}$).
  * Dynamic panic scaling: volume ($0.0 \to 0.95$), heartbeat rate/pitch acceleration ($0.95 \to 1.35$), and elevated panic floor during AI `Chase` state.
  * Layered `PanicBreathingAudioComponent` (`voicebosch-out-of-breath-male-176715`) activating when panic factor exceeds $45\%$.
  * Dispatches `OnPanicFactorUpdated(PanicFactor)` event and subtle FOV pulse for post-process and UI effects.
* **Ceiling Light Distortion & Blackout Envelope (`ABackroomsEntity`):**
  * Discovers all `APointLight` / `UPointLightComponent` units across any active map at `BeginPlay` and caches baseline intensities.
  * **Flicker Envelope ($500\text{ cm} \le d \le 1400\text{ cm}$):** Rapid Perlin noise oscillation ($\sim 28\text{ Hz}$) with spatial positional seeding and random ballast dropouts simulating failing starters.
  * **Blackout Envelope ($d < 500\text{ cm}$):** Lights directly around the entity extinguish into deep shadows with rare faint sparks ($<6\%$), shrouding the monster in a moving pocket of darkness.
  * **Smooth Recovery ($d > 1400\text{ cm}$):** Lights interpolate back to baseline intensity via `FMath::FInterpTo(..., 8.0f)`.
  * Clean `EndPlay` restoration guarantees zero permanently dimmed lights on entity despawn or level transition.

## Audio & Physics
* **Sound Classes & Mix:**
  * Root Sound Class: `SC_Master` located in `/Game/Backrooms/Audio/SoundClasses/SC_Master`.
  * Child Sound Classes: `SC_SFX` (player footsteps, physical interaction), `SC_Ambience` (fluorescent lights, HVAC, room tone), `SC_Entity` (monster footsteps, heavy stomps, sickly breathing, growls).
  * Sound Mix: `SM_BackroomsVolume` (`/Game/Backrooms/Audio/SM_BackroomsVolume`) with runtime volume overrides applied smoothly via `UGameplayStatics::SetSoundMixClassOverride`.
* **Physical Materials:**
  * `PM_Carpet` (`Friction=0.75`, `StaticFriction=0.75`, `SurfaceType=SurfaceType1` "Carpet") assigned to `M_Backrooms_Carpet` and inherited by `MI_Backrooms_Carpet`.
  * `PM_Concrete` (`SurfaceType=SurfaceType2` "Concrete") assigned to `MI_Level2_Floor_Tile` for sharp industrial footstep playback.
* **Player Footstep Audio:** Implemented natively in `ABackroomsCharacter` with distance-based stride accumulation (`BaseStepDistance=150cm`, `SprintStepDistance=180cm`), ground line-tracing with physical material evaluation, random pitch modulation ($0.95$–$1.05$), velocity-interpolated volume ($0.4$ walk, $0.7$ sprint), and soft Berber carpet samples (`Step1`–`Step5` in `Content/Backrooms/Audio/Foot-steps/Footsteps/`) with non-repeating shuffle selection. Default fallback to concrete steps (`freeeverythingxx-walking-on-concrete-ver-2-268513`). Routed through `SC_SFX`.
* **Entity Footstep & Breathing Audio System:** Implemented natively in `ABackroomsEntity`, routed through `SC_Entity`:
  * **Dynamic Cadence & Stride:** Stride distance and timing dynamically vary by operational AI state:
    * *Idle Wander* ($180\text{ cm/s}$): $175\text{ cm}$ stride ($\sim 1.0\text{ s}$ per step), volume $0.55$, deep ominous pitch ($0.80$–$0.88$).
    * *Search* ($240\text{ cm/s}$): $145\text{ cm}$ stride ($\sim 0.6\text{ s}$ per step), volume $0.72$, tense calculated pitch ($0.78$–$0.85$).
    * *Chase* ($450\text{ cm/s}$): $125\text{ cm}$ stride ($\sim 0.28\text{ s}$ per step), volume $1.05$, aggressive heavy pitch ($0.72$–$0.80$), layered with heavy monster stomps (`lordsonny-monster-footstep-162883`, `universfield-heavy-walking-footsteps-352771`).
  * **Spatialized 3D Attenuation:** Custom `USoundAttenuation` with NaturalSound falloff ($3500\text{ cm}$ / $35\text{ m}$), distance Low-Pass Filter ($600$–$3500\text{ cm}$ from $20\text{ kHz}$ to $800\text{ Hz}$), and visibility-channel wall occlusion ($350\text{ Hz}$ LPF, $0.40$ volume through walls).
  * **Ambient Sickly Breathing:** Attached `BreathingAudioComponent` (`freesound_community-sickly-breathing-83152`) with 3D attenuation ($2500\text{ cm}$ falloff, LPF, occlusion), volume dynamically scaling across states ($0.35$ Wander $\to$ $0.55$ Search $\to$ $0.85$ Chase).
  * **Surface Detection:** Ground downward line trace with physical material check (`PM_Carpet` / `SurfaceType1`). Non-repeating shuffle selection across sound sets.

## Atmosphere & Audio Subsystems
* **Atmosphere Manager (`ABackroomsAtmosphereManager`):** C++ actor managing persistent ambient dread drone layers and geometry-aware 3D exploration horror cues (door knocks at dead-ends, distant door slams, heavy reverb impacts, blind corner whispers, anti-freeze tension clocks).
* **Sound Classes:**
  * `SC_Master`: Global master volume control.
  * `SC_Ambience`: Persistent drones, room tones, fluorescent hums, and spatialized environment cues (strictly capped at $\le 0.5$ volume).
  * `SC_Entity`: Monster footsteps, vocalizations, attacks, and sight stingers.
  * `SC_SFX`: Player footsteps, flashlight clicks, and interactable props.
* **Asset Manager Soft Pointers:** Audio assets registered in `UBackroomsAssetManager` for asynchronous/synchronous loading and asset validation via `AssetRegistry`.

## Camera & Viewport Collision
* **Native C++ Collision Prevention:** Implemented directly in `ABackroomsCharacter::UpdateCameraCollision(DeltaTime)` evaluated each `Tick`.
* **Horror Immersion & Anti-Scouting:** Prevents camera near-clipping plane from penetrating wall corners, doorway frames, and corridor intersections, completely eliminating player exploits to scout AI entity locations through geometry.
* **Sphere Sweep Geometry:**
  * Probe Radius: `CameraProbeRadius = 12.0f` ($12\text{ cm}$ sphere sweeping along `ECC_Camera`).
  * Head Pivot: `BaseHeadRelativeLocation = (0.0f, 0.0f, 64.0f)` (or skeletal mesh `head` socket location).
  * Desired Forward Offset: `DefaultCameraOffset = (15.0f, 0.0f, 0.0f)` forward from head.
  * Safe Clamp Distance: Clamped between `CameraMinDistance = 10.0f` (and $0\text{ cm}$ on extreme corner penetration) and `DefaultCameraOffset.Size()`.
  * Smoothing Dynamics: `FMath::VInterpTo` with `CameraSmoothSpeed = 10.0f` ($\approx 15\%$ per frame at 60 FPS) ensures zero jarring camera snaps when entering or exiting wall proximity.
* **Debug Visualization:** Reflected property `bDrawDebugCameraCollision` for in-editor trace line and sphere visualization (compiled out in Shipping).

## Input
**Stack:** Enhanced Input  
**Mapping contexts:** `/Game/Input/IMC_Default`, `/Game/Input/IMC_Player`, `/Game/Input/IMC_MouseLook`  
**Input Actions:**
* `IA_Move`: 2D Vector (WASD / Left Stick).
* `IA_Look`: 2D Vector (Mouse / Right Stick) with sensitivity scaling (`MouseSensitivity` default `0.20f`) and Y-axis inversion.
* `IA_Sprint`: Free ungated sprint toggle / hold.
* `IA_Jump`: Jump impulse.
* `IA_Pause`: Digital bool with `bTriggerWhenPaused=True`, mapped to `Escape` (Keyboard) and `Gamepad_Special_Right` (Start button).
* **Sensitivity Calibration:** Enhanced Input look multiplier defaults to `0.20f` across `ABackroomsCharacter` and `UBackroomsSaveGame`, with automatic migration of legacy `1.0f` saves.
**Added by:** `BP_BackroomsPlayerController` on possession.

## AI & Navigation
* **Architecture:** Native `ABackroomsAIController` with derived Blueprint `BP_BackroomsAIController` controlling `BP_BackroomsEntity`.
* **Type-Safe Blackboard Selectors:** Eliminates fragile string literals via reflected `FBlackboardKeySelector` properties:
  * `AIStateKey`: Enum filter constrained to `EBackroomsAIState` (`Idle`, `Wander`, `Chase`, `Search`).
  * `TargetActorKey`: Object filter constrained to `AActor` (`TargetPlayer`).
  * `HasTargetKey`: Bool filter.
  * `TargetLocationKey`: Vector filter for reachable random and search destination points.
  * Automatic key resolution on possess via `ResolveSelectedKey(*BBData)` with `IsNone()` fallbacks.
* **Perception:** Forward sight cone ($45^\circ$, $18\text{ s}$ memory), speed scaling ($180\text{ cm/s}$ wander, $450\text{ cm/s}$ chase).
* **Navigation:** `NavMeshBoundsVolume` covering playable corridor/room geometry (`NavMeshBoundsVolume_0` in `L_Backrooms_Main`; required in `backroomsLevel2` with Agent Radius `42`, Agent Height `192`, Cell Size `19`) with native pathing via `AIModule` and `NavigationSystem`.

## Lighting & Rendering
* **RHI:** Default DirectX 11 (`PCD3D_SM5`) with DirectX 12 (`PCD3D_SM6`) fallback/support; Substrate materials intentionally disabled (`r.Substrate=False`) for <2GB VRAM Low-tier target compatibility and DX11 fallback.
* **GI & Shadows:** Lumen dynamic indirect bounce ($1.5$) & diffuse boost ($1.3$); Virtual Shadow Maps enabled on High/Epic tiers (disabled on Low/Medium per `DefaultScalability.ini`).
* **Exposure:** Unbound `PPV_Backrooms` (`AEM_Histogram`, EV100 bounds `[0.0, 5.0]`, bias $+0.75$).
* **Corridor Lights:** 216x warm fluorescent PointLights ($300\text{ cd}$, $950\text{ radius}$) and `BP_FlickeringLight` zero-tick fixtures.
* **Scalability:** Custom `DefaultScalability.ini` configured for low-end / <2GB VRAM targets (Lumen/VSM disabled on Low/Medium, texture pools bounded to 512MB/768MB, TSR resolution scaling).

## Source control
**VCS:** Git (`master` branch) with Git LFS for binary `.uasset` / `.umap` assets.
