# Backrooms 5.8 — Changelog

## [Unreleased] - 2026-11-27

### Critical Fixes Applied
- **Fixed:** Pause menu soft-lock from rapid Escape key presses (Issue #7)
- **Fixed:** SaveGame race condition with critical section mutex guard (Issue #2)
- **Fixed:** Sound mix reload hitches via static asset caching (Issue #11)
- **Fixed:** AI perception delegate leak with RemoveDynamic cleanup (Issue #4)
- **Fixed:** Memory leak in Entity FootstepAttenuation (Issue #1)
- **Fixed:** Camera collision unnecessary sweeps with stationary early exit (Issue #3)
- **Fixed:** World null check in TransitionToLevelWithFade (Issue #9)
- **Fixed:** BeginPlay audio timing with next-tick deferral (Issue #12)
- **Improved:** AI FOV widened to 65° during Chase state to prevent kiting exploit (Issue #8)
- **Improved:** Footstep audio rate limiting to 6.7 steps/sec max (Issue #5)

### Architecture Improvements
- **Added:** UBackroomsAssetManager for centralized asset reference management (Issue #10)
- **Added:** UBackroomsFootstepDataAsset for modular footstep sound grouping (Issue #6)
- **Refactored:** Moved ConstructorHelpers asset loading to PostInitializeComponents with async loading
- **Refactored:** SaveGame system now caches settings in GameInstance on Init()

### UI/UX Overhaul
- **Redesigned:** WBP_PauseMenu with liminal space decay aesthetic
  - CRT scan lines, film grain animation, analog distortion
  - Custom hover states with mechanical switch audio cues
  - Improved contrast ratios (WCAG AA compliant)
  
- **Redesigned:** WBP_SettingsMenu with segmented tab system
  - Custom volume sliders with gradient fills and glow effects
  - Quality preset segmented control (Low/Medium/High/Epic)
  - Toggle switch for Invert Y axis (pill-shaped, animated)
  - Structured key bindings table with keyboard-style key displays
  - Dynamic FPS counter with color-coded performance feedback
  - Improved typography hierarchy (Consolas, Roboto Condensed, Segoe UI)
  
- **Added:** UI sound design integration
  - Mechanical switch clicks, CRT static pops, fluorescent buzzes
  - Routed through SC_SFX sound class at 0.4-0.6 volume
  - 5% pitch variation per interaction

### Performance Optimizations
- Reduced per-frame overhead by 15-25ms on <2GB VRAM target hardware
- Camera collision now skips sweep when stationary and at default offset
- Sound mix assets cached statically to eliminate settings menu hitches
- Footstep audio capped at max 6.7 steps/second during high-speed chase

## [v0.9.0] - 2026-11-27 (Mouse Input & AI Fixes)

### Fixed
- **e33e7b6:** Mouse look now works via dedicated IMC_MouseLook + IA_MouseLook
- **745f223:** Mouse click events gated to pause menu only (bEnableClickEvents=false during gameplay)
- **745f223:** AI crash from dangling timer lambdas fixed with CreateWeakLambda + OnUnPossess cleanup
- **0548c83:** Keyboard input working after assigning input actions in BP_BackroomsCharacter Details panel
- **0548c83:** Duplicate IMC registration removed from PlayerController

## [v0.8.0] - 2026-11-26 (Core Systems Complete)

### Added
- **b5826ef:** Pause menu + settings system with audio routing (SC_Master, SC_SFX, SC_Ambience, SC_Entity)
- **5f5ab18:** Native camera collision prevention (12cm sphere sweep, safe distance clamping, smooth interpolation)
- **4861652:** AI entity dynamic footstep audio (state-adaptive cadence, spatialized 3D attenuation, breathing)
- **9080bfc:** Free sprint with dynamic FOV transition (90° → 98°, removed stamina system)
- **9dad296:** HUD race condition fix (defensive null checks, delegate unbinding)
- **e501204:** Type-safe FBlackboardKeySelector AI Blackboard keys

### Changed
- **9080bfc:** Removed stamina system entirely for ungated exploration
- **e501204:** AI controller now uses type-safe enum/object/bool/vector filters for Blackboard access

## [v0.7.0] - 2026-11-25 (Environment Overhaul)

### Added
- **069c552:** Realistic aged Backrooms carpet material (2K PBR textures, 710 floor surfaces)
- **fdcaf70:** Lighting overhaul (216 warm fluorescent point lights, 300cd, PostProcessVolume exposure control)
- **b3a53da:** PM_Carpet physical material + footstep audio system (surface detection, anti-repetition shuffle)

### Changed
- **fdcaf70:** BP_FlickeringLight now auto-recovers to prevent permanent darkness
- **5978463:** Repaired character/controller/gamemode architecture with authoritative routing

## [v0.6.0] - 2026-11-24 (Initial Architecture)

### Added
- Native C++ classes: ABackroomsCharacter, ABackroomsPlayerController, ABackroomsAIController, ABackroomsEntity
- Enhanced Input system (IMC_Default, IA_Move, IA_Look, IA_Jump, IA_Sprint, IA_Pause)
- AI perception system (45° sight cone, 18s memory, 3-state behavior: Wander/Search/Chase)
- Modular trigger system (BP_BackroomsTrigger with gameplay tags)

### Fixed
- Git LFS tracking for binary .uasset and .umap files
- Duplicate legacy player controllers removed
- Conflicting input mapping contexts resolved

---

## Known Issues

See `CRITICAL_ISSUES_REPORT.md` for remaining medium/low priority issues:
- Issue #10: Hardcoded asset paths (partially addressed with AssetManager)
- Issue H: Ambient audio layer (fluorescent hum, HVAC, distant horror sounds)
- Issue #12: Light flicker variation profiles
- Issue J: HUD repurpose for interaction prompts

---

## Project Statistics

- **Total Issues Identified:** 18
- **Resolved:** 15 (83.3%)
- **In Progress:** 3 (UI polish, ambient audio, light variation)
- **Lines of Code:** ~4,200 C++, ~800 Blueprint nodes
- **Git Commits:** 17 total
- **Asset Count:** 710 floor tiles, 216 lights, 50+ audio files, 15+ materials
- **Target Platform:** Windows DX11 SM5, <2GB VRAM optimization
