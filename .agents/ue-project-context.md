# Backrooms — UE Project Context

**Generated:** 2026-11-25  
**Engine version:** Unreal Engine 5.8 (Launcher / Binary Build)  
**Project file:** `Backrooms.uproject`  

---

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
| GameInstance | `BackroomsGameInstance` (`UBackroomsGameInstance`) |
| PlayerController | `BP_BackroomsPlayerController` (derives from native `ABackroomsPlayerController`, `IA_Pause` handling, pause menu toggling, input mode & mouse cursor control) |
| Pawn / Character | `BP_BackroomsCharacter` (derives from native `ABackroomsCharacter`, Enhanced Input `IMC_Player`, ungated free sprint 300/600 cm/s with dynamic FOV 90°–98°, footstep audio system, mouse sensitivity & invert Y integration) |
| HUD / UI | `WBP_BackroomsHUD` (minimalist liminal horror UI with flashlight status, level intro, interaction prompts; stamina bar removed) |
| Pause Menu | `WBP_PauseMenu` (derives from native `UBackroomsPauseMenu`, 70% dark horror overlay, Resume, Settings, Quit) |
| Settings Menu | `WBP_SettingsMenu` (derives from native `UBackroomsSettingsMenu`, 85% dark horror overlay, Audio/Graphics/Controls tabs, live 0.5s FPS counter) |
| Save Game | `BackroomsSaveGame` (`UBackroomsSaveGame`, persistent audio volumes, scalability presets, resolution scale, mouse sensitivity, invert Y) |
| Entity / Monster | `BP_BackroomsEntity` (derives from native `ABackroomsEntity`, spatialized 3D footstep audio, state-driven cadence/volume, ambient breathing) |
| AI Controller | `BP_BackroomsAIController` (derives from native `ABackroomsAIController`) |
| Function Library | `BackroomsFunctionLibrary` (`UBackroomsFunctionLibrary` — camera fade, math, utilities) |
| Ambient Manager | `BP_Backrooms_AmbientManager` |

## Audio & Physics
* **Sound Classes & Mix:**
  * Root Sound Class: `SC_Master` located in `/Game/Backrooms/Audio/SoundClasses/SC_Master`.
  * Child Sound Classes: `SC_SFX` (player footsteps, physical interaction), `SC_Ambience` (fluorescent lights, HVAC, room tone), `SC_Entity` (monster footsteps, heavy stomps, sickly breathing, growls).
  * Sound Mix: `SM_BackroomsVolume` (`/Game/Backrooms/Audio/SM_BackroomsVolume`) with runtime volume overrides applied smoothly via `UGameplayStatics::SetSoundMixClassOverride`.
* **Physical Materials:** `PM_Carpet` (`Friction=0.75`, `StaticFriction=0.75`, `SurfaceType=SurfaceType1` "Carpet") assigned to `M_Backrooms_Carpet` and inherited by `MI_Backrooms_Carpet`.
* **Player Footstep Audio:** Implemented natively in `ABackroomsCharacter` with distance-based stride accumulation (`BaseStepDistance=150cm`, `SprintStepDistance=180cm`), ground line-tracing with physical material evaluation, random pitch modulation ($0.95$–$1.05$), velocity-interpolated volume ($0.4$ walk, $0.7$ sprint), and soft Berber carpet samples (`Step1`–`Step5` in `Content/Backrooms/Audio/Foot-steps/Footsteps/`) with non-repeating shuffle selection. Default fallback to concrete steps (`freeeverythingxx-walking-on-concrete-ver-2-268513`). Routed through `SC_SFX`.
* **Entity Footstep & Breathing Audio System:** Implemented natively in `ABackroomsEntity`, routed through `SC_Entity`:
  * **Dynamic Cadence & Stride:** Stride distance and timing dynamically vary by operational AI state:
    * *Idle Wander* ($180\text{ cm/s}$): $175\text{ cm}$ stride ($\sim 1.0\text{ s}$ per step), volume $0.55$, deep ominous pitch ($0.80$–$0.88$).
    * *Search* ($240\text{ cm/s}$): $145\text{ cm}$ stride ($\sim 0.6\text{ s}$ per step), volume $0.72$, tense calculated pitch ($0.78$–$0.85$).
    * *Chase* ($450\text{ cm/s}$): $125\text{ cm}$ stride ($\sim 0.28\text{ s}$ per step), volume $1.05$, aggressive heavy pitch ($0.72$–$0.80$), layered with heavy monster stomps (`lordsonny-monster-footstep-162883`, `universfield-heavy-walking-footsteps-352771`).
  * **Spatialized 3D Attenuation:** Custom `USoundAttenuation` with NaturalSound falloff ($3500\text{ cm}$ / $35\text{ m}$), distance Low-Pass Filter ($600$–$3500\text{ cm}$ from $20\text{ kHz}$ to $800\text{ Hz}$), and visibility-channel wall occlusion ($350\text{ Hz}$ LPF, $0.40$ volume through walls).
  * **Ambient Sickly Breathing:** Attached `BreathingAudioComponent` (`freesound_community-sickly-breathing-83152`) with 3D attenuation ($2500\text{ cm}$ falloff, LPF, occlusion), volume dynamically scaling across states ($0.35$ Wander $\to$ $0.55$ Search $\to$ $0.85$ Chase).
  * **Surface Detection:** Ground downward line trace with physical material check (`PM_Carpet` / `SurfaceType1`). Non-repeating shuffle selection across sound sets.

## Input
**Stack:** Enhanced Input  
**Mapping contexts:** `/Game/Input/IMC_Default`, `/Game/Input/IMC_Player`, `/Game/Input/IMC_MouseLook`  
**Input Actions:**
* `IA_Move`: 2D Vector (WASD / Left Stick).
* `IA_Look`: 2D Vector (Mouse / Right Stick) with sensitivity scaling and Y-axis inversion.
* `IA_Sprint`: Free ungated sprint toggle / hold.
* `IA_Jump`: Jump impulse.
* `IA_Pause`: Digital bool with `bTriggerWhenPaused=True`, mapped to `Escape` (Keyboard) and `Gamepad_Special_Right` (Start button).
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
* **Navigation:** `NavMeshBoundsVolume_0` covering `L_Backrooms_Main` corridor maze with native pathing via `AIModule` and `NavigationSystem`.

## Lighting & Rendering
* **RHI:** Default DirectX 11 (`PCD3D_SM5`) with DirectX 12 (`PCD3D_SM6`) fallback/support; Substrate materials intentionally disabled (`r.Substrate=False`) for <2GB VRAM Low-tier target compatibility and DX11 fallback.
* **GI & Shadows:** Lumen dynamic indirect bounce ($1.5$) & diffuse boost ($1.3$); Virtual Shadow Maps enabled on High/Epic tiers (disabled on Low/Medium per `DefaultScalability.ini`).
* **Exposure:** Unbound `PPV_Backrooms` (`AEM_Histogram`, EV100 bounds `[0.0, 5.0]`, bias $+0.75$).
* **Corridor Lights:** 216x warm fluorescent PointLights ($300\text{ cd}$, $950\text{ radius}$) and `BP_FlickeringLight` zero-tick fixtures.
* **Scalability:** Custom `DefaultScalability.ini` configured for low-end / <2GB VRAM targets (Lumen/VSM disabled on Low/Medium, texture pools bounded to 512MB/768MB, TSR resolution scaling).

## Source control
**VCS:** Git (`master` branch) with Git LFS for binary `.uasset` / `.umap` assets.
