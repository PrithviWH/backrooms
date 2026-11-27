# Backrooms — UE Project Context

**Generated:** 2026-11-25  
**Engine version:** Unreal Engine 5.8 (Launcher / Binary Build)  
**Project file:** `Backrooms.uproject`  

---

## Modules
**Primary game module:** `Backrooms`

| Module | Host type | Public deps | Private deps | Notes |
|---|---|---|---|---|
| `Backrooms` | Runtime | `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `AIModule`, `NavigationSystem`, `GameplayTasks` | *none* | Main gameplay and core C++ module (`ABackroomsGameModeBase`, `ABackroomsAIController`, `UBackroomsGameInstance`, `UBackroomsFunctionLibrary`) |

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
| PlayerController | `BP_BackroomsPlayerController` |
| Pawn / Character | `BP_BackroomsCharacter` (derives from native `ABackroomsCharacter`, Enhanced Input `IMC_Player`, ungated free sprint 300/600 cm/s with dynamic FOV 90°–98°, footstep audio system) |
| HUD / UI | `WBP_BackroomsHUD` (minimalist liminal horror UI with flashlight status, level intro, interaction prompts; stamina bar removed) |
| Entity / Monster | `BP_BackroomsEntity` |
| AI Controller | `BP_BackroomsAIController` (derives from native `ABackroomsAIController`) |
| Function Library | `BackroomsFunctionLibrary` (`UBackroomsFunctionLibrary` — camera fade, math, utilities) |
| Ambient Manager | `BP_Backrooms_AmbientManager` |

## Audio & Physics
* **Physical Materials:** `PM_Carpet` (`Friction=0.75`, `StaticFriction=0.75`, `SurfaceType=SurfaceType1` "Carpet") assigned to `M_Backrooms_Carpet` and inherited by `MI_Backrooms_Carpet`.
* **Footstep Audio:** Implemented natively in `ABackroomsCharacter` with distance-based stride accumulation (`BaseStepDistance=150cm`, `SprintStepDistance=180cm`), ground line-tracing with physical material evaluation, random pitch modulation ($0.95$–$1.05$), velocity-interpolated volume ($0.4$ walk, $0.7$ sprint), and soft Berber carpet samples (`Step1`–`Step5` in `Content/Backrooms/Audio/Foot-steps/Footsteps/`) with non-repeating shuffle selection. Default fallback to concrete steps (`freeeverythingxx-walking-on-concrete-ver-2-268513`).

## Input
**Stack:** Enhanced Input  
**Mapping contexts:** `/Game/Input/IMC_Player`  
**Input Actions:** `IA_Move`, `IA_Look`, `IA_Sprint`, `IA_Jump`  
**Added by:** `BP_BackroomsCharacter` on possession.

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
