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
| Pawn / Character | `BP_BackroomsCharacter` (Enhanced Input `IMC_Player`, stamina, head-bob) |
| HUD / UI | `WBP_BackroomsHUD` (`HorrorUI`) |
| Entity / Monster | `BP_BackroomsEntity` |
| AI Controller | `BP_BackroomsAIController` (derives from native `ABackroomsAIController`) |
| Function Library | `BackroomsFunctionLibrary` (`UBackroomsFunctionLibrary` — camera fade, math, utilities) |
| Ambient Manager | `BP_Backrooms_AmbientManager` |

## Input
**Stack:** Enhanced Input  
**Mapping contexts:** `/Game/Input/IMC_Player`  
**Input Actions:** `IA_Move`, `IA_Look`, `IA_Sprint`, `IA_Jump`  
**Added by:** `BP_BackroomsCharacter` on possession.

## AI & Navigation
**Stack:** `GameplayStateTree` + AI Perception (Sight and Hearing)  
**Navigation:** `NavMeshBoundsVolume_0` covering `L_Backrooms_Main` corridor maze. Native pathing supported via `AIModule` and `NavigationSystem`.

## Lighting & Rendering
* **RHI:** DirectX 12 (`PCD3D_SM6`), Substrate materials enabled (`r.Substrate=True`), Ray Tracing enabled.
* **GI & Shadows:** Lumen dynamic indirect bounce ($1.5$) & diffuse boost ($1.3$); Virtual Shadow Maps enabled.
* **Exposure:** Unbound `PPV_Backrooms` (`AEM_Histogram`, EV100 bounds `[0.0, 5.0]`, bias $+0.75$).
* **Corridor Lights:** 216x warm fluorescent PointLights ($300\text{ cd}$, $950\text{ radius}$) and `BP_FlickeringLight` zero-tick fixtures.
* **Scalability:** Custom `DefaultScalability.ini` configured for low-end / <2GB VRAM targets (Lumen/VSM disabled on Low/Medium, texture pools bounded to 512MB/768MB, TSR resolution scaling).

## Source control
**VCS:** Git (`master` branch) with Git LFS for binary `.uasset` / `.umap` assets.
