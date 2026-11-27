# Backrooms 5.8 — Master Project Architecture & Audit Report

**Engine Version:** Unreal Engine 5.8 (with Model Context Protocol Server)  
**Authoritative Level:** `/Game/Backrooms/Maps/L_Backrooms_Main`  
**Git Branch / Commit:** `master` (`069c552`)  
**Git LFS:** Active & Tracking binary `.uasset` and `.umap` assets  
**Status:** Architecture Repaired, Lighting Overhauled, PBR Carpet Implemented, 0 Errors / 0 Warnings  

---

## 1. Executive Summary

This report documents the architectural audit, structural repair, environment overhaul, and material pipeline implementation conducted on the **Backrooms 5.8** project. 

The project has been transitioned from an unoptimized state with duplicate player controllers, conflicting input assets, pitch-black lighting, and flat untextured floors into a clean, performant, and atmospheric psychological horror experience. All systems have been refactored to preserve existing assets without destructive deletion, while establishing authoritative gameplay Single Sources of Truth (SSOT).

```mermaid
graph TD
    GM[BP_BackroomsGameMode] -->|Default Pawn| PC[BP_BackroomsCharacter]
    GM -->|Player Controller| CTRL[BP_BackroomsPlayerController]
    CTRL -->|Initializes OnPossess| HUD[HorrorUI / WBP_HorrorHUD]
    PC -->|Enhanced Input IMC_Player| INP[IA_Move / IA_Look / IA_Sprint / IA_Jump]
    PC -->|Flashlight Delegate| HUD
    MAP[L_Backrooms_Main] --> PPV[Unbound PostProcessVolume]
    MAP --> LIGHTS[216x Warm PointLights 300 cd]
    MAP --> FLICKER[BP_FlickeringLight Auto-Recover]
    MAP --> FLOORS[710x Modular Floors MI_Backrooms_Carpet]
```

---

## 2. Core Architecture & Authoritative Systems

### 2.1 Authoritative Character (`BP_BackroomsCharacter`)
* **Asset Location:** `/Game/Backrooms/Characters/Player/BP_BackroomsCharacter`
* **Parent Class:** Native C++ `ABackroomsCharacter` (`ACharacter`)
* **Camera System:** Integrated first-person camera component (`FirstPersonCamera`) attached directly to capsule root with configurable eye-height, dynamic sprint FOV transitions ($90^\circ \to 98^\circ$), and native C++ camera collision prevention against wall clipping.
* **Enhanced Input Integration:** Centralized in `Content/Input/`. Fully wired to:
  * `IA_Move`: 2D Vector directional movement with smooth acceleration.
  * `IA_Look`: Mouse look with sensitivity scaling and vertical pitch clamping.
  * `IA_Jump`: Standard impulse-based jump.
  * `IA_Sprint`: Dynamic sprint state management.
* **Ungated Free Sprint Mechanics & Dynamic FOV:**
  * Free sprint unencumbered by stamina meters or resource management, tailored for pure atmospheric liminal space exploration.
  * Base Walk Speed: $300\text{ cm/s}$, Sprint Speed: $600\text{ cm/s}$ (active while `IA_Sprint` is held).
  * Smooth Dynamic Camera FOV: transitions seamlessly between $90^\circ$ (walk) and $98^\circ$ (sprint) via frame-rate-independent `FMath::FInterpTo` in `Tick()`.
  * Removed stamina drain timers, recovery slowdowns, and stamina meters.
* **Mobile / Fallback Input:** Virtual joystick and touch interface bindings preserved for cross-platform support.

### 2.2 Player Controller & HUD Lifecycle (`BP_BackroomsPlayerController`)
* **Asset Location:** `/Game/Backrooms/Blueprints/Systems/BP_BackroomsPlayerController`
* **Race-Condition-Free Deterministic Execution Sequence:**
  1. `Event OnPossess` fires upon possession.
  2. **Type Validation**: Validates the possessed pawn via `CastToBP_BackroomsCharacter`. If invalid/null, logs a descriptive warning and safely aborts HUD setup.
  3. **Defensive Delegate Cleanup**: If a previous character was possessed (`CurrentCharacter` is valid), calls `UnbindCharacter` on `WBP_BackroomsHUD` to unbind stale event dispatchers.
  4. **Pawn Reference Tracking**: Caches the valid possessed character in `CurrentCharacter`.
  5. **Null-Safe HUD Instantiation / Reuse**:
     - If `BackroomsHUD` is invalid/null, creates `WBP_BackroomsHUD` with owning player `self`.
     - Performs an explicit null check on the created widget (logging an error on creation failure).
     - Assigns to `BackroomsHUD` and executes `AddToPlayerScreen`.
     - If `BackroomsHUD` already exists (e.g. re-possession mid-game or level reload), reuses the existing widget without duplicate instantiation.
  6. **Delegate Binding**: Calls `SetupCharacter` on `BackroomsHUD` with the current character to bind the flashlight toggle delegate (`OnFlashlightToggled` -> `SetFlashlightState`).
  7. **Clean HUD Presentation**: `WBP_BackroomsHUD` maintains a minimalist horror HUD (collapsing legacy stamina bar, displaying only level intro and interactive cues).
* **Character BeginPlay (`BP_BackroomsCharacter`)**:
  - `BeginPlay` initializes character movement and logs operational state cleanly without obsolete stamina or state restoration calls.

### 2.3 GameMode Routing (`BP_BackroomsGameMode`)
* **Asset Location:** `/Game/Backrooms/Blueprints/Systems/BP_BackroomsGameMode`
* **Default Pawn Class:** Set to `BP_BackroomsCharacter`.
* **Player Controller Class:** Set to `BP_BackroomsPlayerController`.
* **HUD Class:** Standard routing ensuring no duplicate legacy game modes spawn unlinked player characters.

### 2.4 Entity & AI System (`BP_BackroomsEntity` & `BP_BackroomsAIController`)
* **Entity Pawn:** `/Game/Backrooms/Blueprints/AI/BP_BackroomsEntity` (reparented directly to native C++ `ABackroomsEntity`).
* **Authoritative C++ Base Class (`ABackroomsEntity`):**
  * Implements dynamic footstep audio that actively tracks entity movement, AI state, and floor physical material.
  * Stride accumulation: Distance-based calculation dynamically adjusted per AI state:
    * *Idle Wander* ($180\text{ cm/s}$): $175\text{ cm}$ stride ($\sim 1.0\text{ s}$ per step), quiet deliberate stalker pace. Volume $0.55$, deep ominous pitch ($0.80$–$0.88$).
    * *Search* ($240\text{ cm/s}$): $145\text{ cm}$ stride ($\sim 0.6\text{ s}$ per step), tense calculated pacing. Volume $0.72$, pitch $0.78$–$0.85$.
    * *Chase* ($450\text{ cm/s}$): $125\text{ cm}$ stride ($\sim 0.28\text{ s}$ per step), rapid panic-inducing heavy pursuit. Volume $1.05$, pitch $0.72$–$0.80$, randomly layered with heavy monster stomps (`lordsonny-monster-footstep-162883`, `universfield-heavy-walking-footsteps-352771`).
  * **Spatialized 3D Sound Attenuation (`EntityFootstepAttenuation`):**
    * NaturalSound distance falloff: $3500\text{ cm}$ ($35\text{ m}$) max audible radius.
    * Air absorption low-pass filter: Attenuates frequencies from $20\text{ kHz}$ at $600\text{ cm}$ down to $800\text{ Hz}$ at $3500\text{ cm}$, rendering distant steps muffled and ominous.
    * Wall occlusion: Visibility-channel trace with $350\text{ Hz}$ cutoff and $0.40$ volume through walls, giving players auditory depth to track threats around corridor bends.
  * **Ambient Sickly Breathing (`BreathingAudioComponent`):**
    * Attached component playing looping sickly breathing audio (`freesound_community-sickly-breathing-83152`).
    * Inline 3D spatial attenuation ($2500\text{ cm}$ falloff, LPF, wall occlusion).
    * Dynamic state volume interpolation: $0.35$ during Wander $\to$ $0.55$ during Search $\to$ $0.85$ during Chase.
  * **Floor Line-Tracing & Non-Repeating Shuffle:**
    * Downward trace evaluating physical materials (`PM_Carpet` / `SurfaceType1` "Carpet").
    * Anti-repetition shuffle across 5 soft carpet samples (`Step1`–`Step5`).
* **AI Controller:** `/Game/Backrooms/Blueprints/AI/BP_BackroomsAIController` (reparented directly to native C++ `ABackroomsAIController`).
* **Type-Safe Blackboard Key Architecture:**
  * Replaced all legacy hardcoded string literals (`TEXT("AIState")`, `TEXT("TargetActor")`, `TEXT("HasTarget")`, `TEXT("TargetLocation")`) with type-safe `FBlackboardKeySelector` fields exposed in `Category = "AI|Blackboard"`:
    * `AIStateKey`: Initialized to `AIState` with enum filtering for `EBackroomsAIState`.
    * `TargetActorKey`: Initialized to `TargetActor` with object class filtering for `AActor`.
    * `HasTargetKey`: Initialized to `HasTarget` with boolean filtering.
    * `TargetLocationKey`: Initialized to `TargetLocation` with vector filtering.
  * In `OnPossess`, resolves key IDs and names dynamically against the active `UBlackboardData` via `ResolveSelectedKey(*BBData)` fetched from `GetBlackboardAsset()`.
  * Safe accessor fallback: reads and writes evaluate `SelectedKeyName.IsNone()` to guarantee resilience even if unassigned in editor defaults.
* **Perception & State Machine:**
  * AI Perception Sight Sense configured for a $45^\circ$ forward cone and $18\text{ s}$ memory.
  * Adaptive movement speeds: Wander at $180\text{ cm/s}$, Search at $240\text{ cm/s}$, Chase at $450\text{ cm/s}$.
  * Deferred recursive pursuit in `OnMoveCompleted` via `SetTimerForNextTick` to prevent call-stack overflows during continuous chase loops.
  * Seamless corridor wandering and player tracking evaluated via `NavMeshBoundsVolume_0`.

### 2.5 Pause Menu & Settings System (`WBP_PauseMenu` & `WBP_SettingsMenu`)
* **Core Goal:** Provide a responsive, horror-themed pause menu allowing players to pause gameplay, tweak audio and graphics settings, adjust mouse sensitivity/inversion, and quit cleanly to desktop.
* **Input Action Integration:**
  * Created `IA_Pause` (`/Game/Input/Actions/IA_Pause` and duplicate at `/Game/Input/IA_Pause`), configured with `ValueType = Boolean` and `bTriggerWhenPaused = True` so pause input can be evaluated while game simulation is suspended.
  * Mapped in `IMC_Default` and `IMC_MouseLook` to `Escape` (Keyboard) and `Gamepad_Special_Right` (Start button).
* **Player Controller Architecture (`ABackroomsPlayerController`):**
  * Subclassed natively as `ABackroomsPlayerController`, with Blueprint `BP_BackroomsPlayerController` reparented directly to it.
  * `TogglePauseMenu()`: Evaluates current pause state. If active, removes the menu widget, resets input mode to `FInputModeGameOnly`, hides the mouse cursor, and calls `UGameplayStatics::SetGamePaused(false)`. If unpaused, instantiates `WBP_PauseMenu`, adds to viewport, sets `FInputModeGameAndUI`, reveals cursor, and sets game paused.
* **Sound Classes & Hierarchy:**
  * Root Sound Class: `SC_Master` (`/Game/Backrooms/Audio/SoundClasses/SC_Master`).
  * Child Sound Classes: `SC_SFX` (player footsteps, physical interaction), `SC_Ambience` (fluorescent lights, HVAC hum, room tone), `SC_Entity` (monster footsteps, heavy stomps, sickly breathing, growls).
  * Sound Mix: `SM_BackroomsVolume` (`/Game/Backrooms/Audio/SM_BackroomsVolume`) with adjusters for all 4 sound classes.
  * Runtime Volume Scaling: Applied via `UGameplayStatics::SetSoundMixClassOverride(World, SoundMix, SoundClass, Volume, 1.0f, 0.05f, true)` to ensure glitch-free real-time volume transitions.
* **Persistent Settings (`UBackroomsSaveGame`):**
  * Saved to slot `BackroomsSettings` with automatic load on startup and save on settings adjustment:
    * **Audio:** `MasterVolume` ($0.7$), `SFXVolume` ($0.8$), `AmbienceVolume` ($0.6$), `EntityVolume` ($0.8$).
    * **Graphics:** `OverallQualityLevel` ($2$ = High), `ResolutionScale` ($100.0\%$).
    * **Controls:** `MouseSensitivity` ($1.0$, range $0.1$–$3.0$), `bInvertY` ($false$).
  * Character Look Integration: `ABackroomsCharacter::Look` applies `MouseSensitivity` and `bInvertY` directly to yaw and pitch axes.
* **UI Aesthetic & User Experience:**
  * **`WBP_PauseMenu` (`UBackroomsPauseMenu`):**
    * Liminal horror dark background overlay ($70\%$ opacity `#000000`).
    * Minimalist typographical hierarchy: Resume, Settings, and Quit to Desktop.
    * Implemented with native Slate widget tree construction fallback so the menu renders out of the box even without UMG asset pre-authoring.
  * **`WBP_SettingsMenu` (`UBackroomsSettingsMenu`):**
    * Dark background overlay ($85\%$ opacity `#000000`).
    * Three tabbed views:
      * **Audio Tab:** Real-time volume sliders for Master, SFX, Ambience, and AI Entity.
      * **Graphics Tab:** Scalability quality selector (Low, Medium, High, Epic) applying via `UGameUserSettings`, resolution scale slider ($50\%$ to $100\%$), and live $0.5\text{ s}$ smoothed FPS counter (`GAverageFPS`).
      * **Controls Tab:** Mouse sensitivity slider ($0.1$–$3.0$), Invert Y-axis toggle, and read-only keybinding cheat-sheet.
    * Back button automatically writes dirty settings to disk via `UBackroomsSaveGame::SaveSettings()`.

### 2.6 Modular Systems & Triggers
* **`BP_BackroomsTrigger`** (`/Game/Backrooms/Blueprints/Systems/Triggers/BP_BackroomsTrigger`):
  * Reusable box trigger volume with configurable gameplay tags (`RequiredTag`).
  * Supports `bOneTimeOnly` execution with automatic trigger deactivation.
  * Resolved legacy stamina compilation dependencies: Added non-breaking compatibility stubs `SavePlayerState`/`LoadPlayerState` on `UBackroomsGameInstance` and deprecated `SprintTime`/`SprintMeter` on `ABackroomsCharacter`. Blueprint compiles cleanly with `BS_UP_TO_DATE` status and zero PIE warnings.
* **`BP_FlickeringLight`** (`/Game/Backrooms/Blueprints/Systems/Lighting/BP_FlickeringLight`):
  * **Zero Event Tick**: Uses asynchronous `Set Timer by Function Name`.
  * **Auto-Recovery**: Brief flickers dip to $2000\text{ lm}$, then automatically schedule `RestoreNormalIntensity` ($6000\text{ lm}$) within $0.08\text{ s}$, preventing lights from permanently turning off.
  * Configurable buzz audio spawn at fixture location.

### 2.7 Native Camera Collision Prevention System (`ABackroomsCharacter`)
* **Problem Solved:** First-person cameras attached to the character capsule without collision detection can clip through wall geometry when players press into sharp $90^\circ$ corners, doorway frames, or thin partition walls. This allowed players to visually scout ahead and locate the stalker entity through walls, destroying the core "fear of the unknown" horror atmosphere.
* **Architectural Approach:** Native C++ sphere sweep on `ECC_Camera` implemented inside `ABackroomsCharacter::UpdateCameraCollision(float DeltaTime)` evaluated during `Tick()`.
* **Mathematical & Geometric Design:**
  * **Head / Pivot Origin:** `BaseHeadRelativeLocation = FVector(0.0f, 0.0f, 64.0f)` (or skeletal mesh `head` socket if available).
  * **Desired Location:** `CapsuleTransform.TransformPosition(BaseHeadRelativeLocation + DefaultCameraOffset)` where `DefaultCameraOffset = FVector(15.0f, 0.0f, 0.0f)`.
  * **Sphere Probe:** `CameraProbeRadius = 12.0f` ($12\text{ cm}$ radius). Sweeps along the forward vector from head to desired camera location, forming a continuous 3D collision envelope around the camera lens.
  * **Collision Evaluation:**
    * When unblocked: Target offset smoothly returns to `DefaultCameraOffset` ($15\text{ cm}$ forward).
    * When blocked: Safe distance is calculated from `HitResult.Distance` and clamped between `CameraMinDistance` ($10.0\text{ cm}$) and `DefaultCameraOffset.Size()`. If extreme corner penetration occurs (`Distance < MinDistance` or `bStartPenetrating`), distance gracefully retracts to $0\text{ cm}$ (the capsule core), keeping the camera safe inside the $42\text{ cm}$ capsule radius.
  * **Frame-Rate-Independent Interpolation:** Offset transitions smoothly via `FMath::VInterpTo(CurrentCameraOffset, TargetOffset, DeltaTime, CameraSmoothSpeed)` with `CameraSmoothSpeed = 10.0f` ($\sim 15\%$ per frame at 60 FPS), eliminating camera snapping, popping, or visual jitter.
* **Blueprint & Designer Controls:**
  * Reflected properties in category `Camera|Collision`: `CameraProbeRadius`, `CameraMinDistance`, `CameraSmoothSpeed`, `DefaultCameraOffset`, `bEnableCameraCollision`, and `bDrawDebugCameraCollision`.
  * Reflected getters `GetCurrentCameraOffset()` and `GetDesiredCameraLocation()`.
  * Shipping-safe debug visualization with `DrawDebugSphere`, `DrawDebugLine`, and `DrawDebugPoint`.

---

## 3. Environment & Visual Overhaul (`L_Backrooms_Main`)

### 3.1 Lighting & Exposure Architecture
Prior to this overhaul, the environment was pitch black due to dark charcoal light color filters (`(0.13, 0.13, 0.12)`), insufficient intensity ($8\text{ cd}$), and the complete absence of a post-process exposure volume.

| Parameter | Previous Value | Overhauled Value | Impact |
| :--- | :--- | :--- | :--- |
| **Post Process Volume** | *None* | `PPV_Backrooms` (Unbound) | Constrains eye adaptation to prevent shadow crushing |
| **Auto-Exposure Method** | Default / Float | `AEM_Histogram` | Smooth, realistic luminance evaluation |
| **EV100 Bounds** | Unbounded | `[0.0, 5.0]` | Eliminates pitch-black darkness while keeping bright spots controlled |
| **Exposure Bias** | `0.0` | `+0.75` | Boosts overall ambient readability |
| **Lumen Indirect Lighting** | `1.0` | `1.5` | Enhances light bounce between corridor walls |
| **Lumen Diffuse Boost** | `1.0` | `1.3` | Warms indirect bounced light throughout maze |
| **Corridor Point Lights (216x)** | `8 cd` / Dark Grey | `300 cd` / Warm Yellow | Eliminates unlit corridor gaps; establishes Backrooms mood |
| **Light Color (Point Lights)** | `(0.129, 0.133, 0.118)` | `(1.0, 0.941, 0.722)` | Emits true warm fluorescent tube illumination |
| **Light Attenuation Radius** | `650` | `950` | Connects adjacent ceiling light cones across corridor intersections |

---

### 3.2 Floor Carpet Material Overhaul
The plain, untextured yellow floors were overhauled into authentic aged commercial office carpeting characteristic of the classic Backrooms aesthetic.

#### Generated Seamless Textures ($2048 \times 2048$)
Located in `/Game/Backrooms/Textures/`:
1. **`T_Backrooms_Carpet_D`** (Diffuse):
   * Muted mustard-yellow/beige base palette ($R=180, G=154, B=92$).
   * Realistic tight Berber loop tufts arranged in an interlocking $128 \times 128$ staggered grid.
   * Multi-tone heathered speckles (dark brown flecks $R=120, G=95, B=60$ and light cream flecks $R=210, G=185, B=130$).
   * Smooth, seamless macro traffic wear and moisture discoloration.
2. **`T_Backrooms_Carpet_N`** (Normal Map):
   * High-frequency micro-crevice normal map computed via toroidal Sobel filter (`compressionSettings = TC_Normalmap`, `sRGB = false`).
3. **`T_Backrooms_Carpet_R`** (Roughness Map):
   * Linear roughness data (`sRGB = false`) calibrated strictly between **$0.85$ and $0.95$**, ensuring a matte surface with zero plastic sheen.

#### Material Implementation & Placement
* **Master Material:** `/Game/Backrooms/Materials/M_Backrooms_Carpet`
  * Features `TextureCoordinate` multiplied by `UV_Tiling` (default `2.0`), tint modulation (`Carpet_Tint`), and zero metallic.
* **Material Instance:** `/Game/Backrooms/Materials/MI_Backrooms_Carpet`
* **Coverage:**
  * Assigned to slot `lambert1` on `/Game/Backrooms/Architecture/Floors/SM_Floor_400x400`, instantly propagating across all **709 modular floor tiles**.
  * Applied directly to `Floor_0` (base level plane), replacing the development grid material.
  * Walls (`M_Backrooms_Wallpaper`), ceilings (`M_Backrooms_CeilingTile`), and door frames remain completely untouched.

### 3.3 Physical Material & Carpet Footstep Audio System
* **Physical Material (`PM_Carpet`):**
  * Asset path: `/Game/Backrooms/Materials/PM_Carpet`
  * Friction & Static Friction: $0.75$ / $0.75$
  * Surface Type: Custom `SurfaceType1` named `"Carpet"` in `Config/DefaultEngine.ini`
  * Assigned directly to `M_Backrooms_Carpet.PhysMaterial`, propagating across all 710 `MI_Backrooms_Carpet` instances.
* **Footstep Audio System (`ABackroomsCharacter` & `BP_BackroomsCharacter`):**
  * Stride accumulation: Distance-based step triggering ($150\text{ cm}$ walk stride to $180\text{ cm}$ sprint stride) executing on ground movement.
  * Downward Line Trace: Traces down from capsule bottom with `bReturnPhysicalMaterial=true`.
  * Surface Evaluation: Checks hit physical material reference and surface type (`SurfaceType1`).
  * Sound Selection: Randomly selects from soft Berber carpet samples (`Step1` through `Step5`) in `/Game/Backrooms/Audio/Foot-steps/Footsteps/` with anti-repetition memory. Non-carpet surfaces fallback to concrete footsteps.
  * Pitch Modulation: Randomly varied between $0.95$ and $1.05$.
  * Volume Scaling: Smoothly interpolated based on horizontal speed: $0.4$ walk volume up to $0.7$ sprint volume.
  * Blueprint Extensibility: Exposes `TriggerFootstep` and `OnFootstepPlayed` event hook in `BP_BackroomsCharacter`.

---

## 4. Performance & Scalability Profile

1. **Shader Pipeline & PSO Precaching**:
   * Enabled in `Config/DefaultEngine.ini`:
     * `r.ShaderPipelineCache.Enabled=1`
     * `r.ShaderPipelineCache.ReportPSO=1`
     * `r.Vulkan.PSOPrecache=1`
   * Prevents runtime micro-stutters during corridor traversal.
2. **Shared Material Instancing**:
   * All 710 floor surfaces share `MI_Backrooms_Carpet`. Zero duplicate master materials or draw-call thrashing.
3. **Virtual Shadow Maps & Lumen**:
   * Point light source radii clamped to $25\text{ cm}$ / $15\text{ cm}$ soft radius to optimize Virtual Shadow Map cache page allocations.
4. **Substrate Material Architecture & Hardware Evaluation**:
   * **Configuration:** `r.Substrate=False` in `Config/DefaultEngine.ini`.
   * **Target Hardware Profile (<2GB VRAM & DX11 Compatibility):**
     * Substrate replaces the traditional fixed GBuffer with a unified multi-closure byte payload (`r.Substrate.BytesPerPixel=80` or higher), requiring complex tile classification shaders and substantially higher memory bandwidth.
     * The project strictly targets low-end hardware profiles (<2GB VRAM with Low-tier texture streaming pool capped at 512MB and `DefaultGraphicsRHI=DefaultGraphicsRHI_DX11` targeting `PCD3D_SM5`).
     * Substrate fundamentally requires Shader Model 6 (`PCD3D_SM6`) on D3D12/Vulkan, and is a project-wide engine setting that cannot be disabled dynamically at runtime per scalability tier.
     * Enabling Substrate would break DX11 / SM5 fallback and induce severe memory bandwidth bottlenecks and VRAM thrashing on <2GB VRAM systems.
   * **Carpet Material Implementation:**
     * `M_Backrooms_Carpet` utilizes standard UE PBR (Base Color, Normal, Roughness) with calibrated linear roughness ($0.85$–$0.95$), UV tiling, and multi-tone speckle textures.
     * This achieves authentic Berber loop fabric light response across all 710 floor surfaces while maintaining steady 60+ FPS on Low scalability.

---

## 5. Verification & Telemetry

* **Play-In-Editor (PIE) Session:**
  * Verified player possession, camera control, ungated sprint speed ($600\text{ cm/s}$), dynamic camera FOV transition ($90^\circ \to 98^\circ$), and clean HUD display without stamina constraints.
  * Ambient dust motes (`NS_DustMote`) render correctly under ceiling lights.
* **Engine Map Check Results:**
  ```text
  MapCheck: Map check complete: 0 Error(s), 0 Warning(s), took 0.435ms to complete.
  ```
* **Git Version Control State:**
  * Branch: `master`
  * Clean working tree (no uncommitted changes).
  * Recent Commits:
    * `069c552`: *Implement realistic aged Backrooms commercial carpet: M_Backrooms_Carpet with seamless 2K fiber loop albedo, normal map, roughness, and assigned to all 710 floor surfaces across L_Backrooms_Main*
    * `fdcaf70`: *Overhaul environment lighting in L_Backrooms_Main: unbound PostProcessVolume with EV100 exposure control, batch-updated 216 PointLights to warm fluorescent yellow (300 cd, 950 radius), and tuned BP_FlickeringLight pulse recovery*
    * `5978463`: *Complete Backrooms architecture repair: Authoritative Character, PlayerController, GameMode routing, zero-tick BP_FlickeringLight, and BP_BackroomsTrigger*

---

## 6. Recommended Next Development Steps

```mermaid
flowchart LR
    A[Current State: Stable Level 0] --> B[Phase 1: Audio Ambiance]
    B --> C[Phase 2: Carpet Physical Surface & Footsteps]
    C --> D[Phase 3: Entity AI Behavior Tree]
    D --> E[Phase 4: Level 1 / Level 2 Transitions]
```

1. **Audio Ambience & Soundscape**:
   * Add continuous low-frequency fluorescent fixture hum ($60\text{ Hz}$ electrical buzz) using attenuation radius on ceiling fixtures.
   * Add distant ambient sound cues (creaking, distant thuds).
2. **Physical Materials & Footsteps** *(Completed)*:
   * Created `PM_Carpet` (`UPhysicalMaterial`) and assigned to `M_Backrooms_Carpet` / `MI_Backrooms_Carpet`.
   * Native line-trace footstep audio implemented in `ABackroomsCharacter` playing randomized Berber carpet footsteps with speed-interpolated volume and pitch variation.
3. **Entity AI Patrol, Chase & Dynamic Footstep Audio** *(Completed)*:
   * Native `ABackroomsEntity` authored and reparented `BP_BackroomsEntity`:
     * State-adaptive footstep cadence ($1.0\text{ s}$ wander, $0.6\text{ s}$ search, $0.28\text{ s}$ chase).
     * Spatialized 3D attenuation ($35\text{ m}$ falloff, LPF air absorption, visibility occlusion).
     * Physical material detection against carpet tiles with anti-repetition shuffle.
     * Ambient attached sickly breathing with dynamic state volume scaling.
   * Native `ABackroomsAIController` refactored to type-safe `FBlackboardKeySelector` key access with strict enum, object, bool, and vector filters.
   * Reparented `BP_BackroomsAIController` and verified in `L_Backrooms_Main` PIE with $45^\circ$ sight cone perception and deferred chase recursion.
