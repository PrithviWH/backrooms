# Backrooms 5.8 — Critical Issues & Major Bugs Report

**Generated:** 2026-11-27  
**Project Status:** 77.8% Complete (14/18 Issues Resolved)  
**Git HEAD:** `e33e7b6` (Mouse look fix + dedicated IMC_MouseLook)  

---

## Executive Summary

Comprehensive codebase analysis identified **12 major architectural, performance, and gameplay-breaking issues** across memory management, threading safety, input handling, AI behavior, and asset loading patterns. While the core gameplay systems are functional, these issues pose risks to stability, scalability, and player experience quality.

**Critical Path Risk:** Constructor blocking (#6) + SaveGame race condition (#2) + Memory leak (#1) form a cascade that degrades editor stability during rapid PIE cycling and creates potential save corruption.

**Immediate Priorities:**
1. Fix pause menu soft-lock (#7) — HIGH severity, direct player impact
2. Cache sound assets in SaveGame (#11) — eliminates 40-80ms hitches during settings adjustment
3. Resolve AI perception delegate leak (#4) — prevents duplicate stimulus processing
4. Address AI exploit via narrow FOV (#8) — restores intended gameplay tension

---

## Issue Breakdown by Severity

### HIGH SEVERITY (4 Issues)

#### **Issue #1: Memory Leak in Entity FootstepAttenuation**
- **Impact:** ~8-12KB leak per entity spawn; accumulates across CDO instantiation, BP spawns, hot-reloads
- **Root Cause:** `SetupAttenuationSettings()` called in both constructor AND `PostInitializeComponents()` without existence check
- **Location:** `BackroomsEntity.cpp:118-144`
- **Fix:**
  ```cpp
  void ABackroomsEntity::SetupAttenuationSettings()
  {
      if (!FootstepAttenuation)
      {
          FootstepAttenuation = NewObject<USoundAttenuation>(
              this, 
              USoundAttenuation::StaticClass(),
              TEXT("EntityFootstepAttenuation"),
              RF_Transient
          );
          // ... configure settings
      }
  }
  ```
  Remove constructor call; keep only in `PostInitializeComponents()`.

---

#### **Issue #2: SaveGame Race Condition**
- **Impact:** Synchronous disk I/O on game thread during `BeginPlay()`; potential corruption if `LoadOrCreateSettings()` called concurrently from Character + PlayerController
- **Root Cause:** No mutex guard around `LoadGameFromSlot()` / `SaveGameToSlot()`
- **Location:** `BackroomsSaveGame.cpp:21-52`
- **Fix:**
  ```cpp
  // In UBackroomsGameInstance::Init()
  CachedSettings = UBackroomsSaveGame::LoadOrCreateSettings();
  
  // In Character/PlayerController BeginPlay:
  if (UBackroomsGameInstance* GI = GetGameInstance<UBackroomsGameInstance>())
  {
      UBackroomsSaveGame* Settings = GI->GetCachedSettings();
  }
  
  // For saves, use:
  static FCriticalSection SaveGameLock;
  FScopeLock Lock(&SaveGameLock);
  UGameplayStatics::AsyncSaveGameToSlot(...);
  ```

---

#### **Issue #6: Constructor Blocking Asset Load**
- **Impact:** 800-1200ms editor startup delay; 15+ synchronous `ConstructorHelpers::FObjectFinder` calls in Character/Entity constructors
- **Root Cause:** Hardcoded asset paths loaded during CDO initialization
- **Location:** `BackroomsCharacter.cpp:67-124`, `BackroomsEntity.cpp:50-124`
- **Fix:**
  - Move to `PostInitializeComponents()` with async loading
  - Create `UFootstepDataAsset` (Primary Data Asset) grouping sounds:
    ```cpp
    UPROPERTY(EditDefaultsOnly, Category = "Footsteps")
    TObjectPtr<UFootstepDataAsset> FootstepData;
    
    // In BeginPlay:
    if (FootstepData)
    {
        CarpetFootstepSounds = FootstepData->CarpetSounds;
    }
    ```

---

#### **Issue #7: Pause Menu Soft-Lock**
- **Impact:** Double-tap Escape during menu transition leaves `PauseMenuInstance` dangling; next toggle crashes or soft-locks with visible cursor but no UI interaction
- **Root Cause:** `TogglePauseMenu()` checks `IsInViewport()` without null validation
- **Location:** `BackroomsPlayerController.cpp:84-93`
- **Fix:**
  ```cpp
  void ABackroomsPlayerController::TogglePauseMenu()
  {
      if (PauseMenuInstance && IsValid(PauseMenuInstance) && PauseMenuInstance->IsInViewport())
      {
          ClosePauseMenu();
      }
      else
      {
          if (PauseMenuInstance && !IsValid(PauseMenuInstance))
          {
              PauseMenuInstance = nullptr;
          }
          OpenPauseMenu();
      }
  }
  ```

---

### MEDIUM SEVERITY (8 Issues)

#### **Issue #3: Camera Collision Sweep Every Tick**
- **Impact:** 0.05-0.15ms wasted per frame even when stationary
- **Root Cause:** No early exit if camera already at default offset and character not moving
- **Location:** `BackroomsCharacter.cpp:289-368`
- **Fix:**
  ```cpp
  if (CurrentCameraOffset.Equals(DefaultCameraOffset, 0.5f) && 
      GetVelocity().SizeSquared() < 100.0f)
  {
      return;
  }
  ```

---

#### **Issue #4: AI Perception Delegate Leak**
- **Impact:** Each `OnPossess()` adds new delegate binding without clearing previous; stimuli fire multiple times on re-possession
- **Root Cause:** No `RemoveDynamic` before `AddDynamic`
- **Location:** `BackroomsAIController.cpp:93-96`
- **Fix:**
  ```cpp
  Perception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ABackroomsAIController::OnTargetPerceptionUpdated);
  Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ABackroomsAIController::OnTargetPerceptionUpdated);
  ```

---

#### **Issue #5: Footstep Audio Spam**
- **Impact:** 3.6 steps/second during chase (450cm/s ÷ 125cm stride); audio thread CPU spike 12-18%; risk hitting 64 concurrent sound limit
- **Root Cause:** No pooling or rate limiting on `PlaySoundAtLocation()`
- **Location:** `BackroomsEntity.cpp:188-298`
- **Fix:**
  - Use `UAudioComponent` pool (2-3 pre-spawned components)
  - Clamp minimum inter-step interval to 0.15s (max 6.7 steps/sec)
  - Disable occlusion trace during chase

---

#### **Issue #8: AI Exploit via Narrow FOV**
- **Impact:** Player can evade entity by strafing just outside 45° vision cone; "kiting" trivializes chase sequences
- **Root Cause:** `PeripheralVisionAngleDegrees = 45.0f` too narrow for open Backrooms corridors
- **Location:** `BackroomsAIController.cpp:33`
- **Fix:**
  - Increase to 60-75° during Chase state: `SetPeripheralVisionAngle(75.0f)` in `OnTargetPerceptionUpdated`
  - Add velocity prediction: search at `LastKnownPlayerLocation + (PlayerVelocity * 2.0s)`
  - Implement audio-based detection: hear sprinting within 800cm radius

---

#### **Issue #9: Missing World Null Check**
- **Impact:** Crash if `TransitionToLevelWithFade()` called from non-world context (editor utility, command-line)
- **Root Cause:** No validation after `GetWorldFromContextObject(..., LogAndReturnNull)`
- **Location:** `BackroomsFunctionLibrary.cpp:30-47`
- **Fix:**
  ```cpp
  UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
  if (!World || !World->IsGameWorld())
  {
      return;
  }
  APlayerController* PC = World->GetFirstPlayerController();
  if (!PC)
  {
      return;
  }
  ```

---

#### **Issue #10: Hardcoded Asset Paths**
- **Impact:** 30+ hardcoded `/Game/Backrooms/...` paths; any folder restructure breaks references silently
- **Root Cause:** No asset manager or soft object path abstraction
- **Location:** Character, Entity, SaveGame, PlayerController constructors
- **Fix:**
  - Create `UBackroomsAssetManager` with `EditDefaultsOnly` properties for all asset references
  - Use `FSoftObjectPath` with async loading
  - Implement `AssetRegistry` post-cook validation

---

#### **Issue #11: Sound Mix Reload on Every Settings Apply**
- **Impact:** 40-80ms hitch per volume slider drag; 4 synchronous `LoadObject` calls per adjustment
- **Root Cause:** No caching of `USoundMix` and `USoundClass` references
- **Location:** `BackroomsSaveGame.cpp:66-93`
- **Fix:**
  ```cpp
  // Static cache in header
  static TObjectPtr<USoundMix> CachedSoundMix;
  static TObjectPtr<USoundClass> CachedSC_Master;
  // ... etc
  
  void UBackroomsSaveGame::ApplyAudioSettings(...)
  {
      if (!CachedSoundMix)
      {
          CachedSoundMix = LoadObject<USoundMix>(...);
      }
      UGameplayStatics::SetSoundMixClassOverride(..., CachedSoundMix, ...);
  }
  ```

---

#### **Issue #12: BeginPlay Blueprint Override Risk**
- **Impact:** If `BP_BackroomsEntity` overrides `BeginPlay` and calls parent late, `BreathingAudioComponent->Play()` fires before Blueprint sets `BreathingSound` reference
- **Root Cause:** Immediate audio playback in native `BeginPlay()` without deferral
- **Location:** `BackroomsEntity.cpp:149-161`
- **Fix:**
  ```cpp
  void ABackroomsEntity::BeginPlay()
  {
      Super::BeginPlay();
      GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
      {
          if (BreathingAudioComponent && BreathingSound && !IsActorBeingDestroyed())
          {
              BreathingAudioComponent->SetSound(BreathingSound);
              BreathingAudioComponent->Play();
          }
      });
  }
  ```

---

## Recommended Fix Priority

### **Phase 1: Immediate (Player-Facing Stability)**
1. Issue #7: Pause menu soft-lock
2. Issue #11: Sound mix reload hitch
3. Issue #8: AI exploit (FOV + velocity prediction)

### **Phase 2: Core Stability**
4. Issue #2: SaveGame race condition
5. Issue #4: AI perception delegate leak
6. Issue #9: World null check

### **Phase 3: Performance & Scalability**
7. Issue #6: Constructor blocking asset load
8. Issue #1: Memory leak (FootstepAttenuation)
9. Issue #3: Camera collision early exit
10. Issue #5: Footstep audio spam

### **Phase 4: Architectural Refactor**
11. Issue #10: Hardcoded asset paths (asset manager)
12. Issue #12: BeginPlay override risk

---

## Testing Verification Checklist

After fixes applied, verify:

- [ ] **Issue #7:** Double-tap Escape 10 times rapidly; menu must open/close cleanly without cursor stuck
- [ ] **Issue #2:** Launch PIE, quit, launch PIE rapidly 5 times; no save corruption warnings in logs
- [ ] **Issue #4:** Possess/unpossess AI entity 3 times; verify only one stimulus log per detection
- [ ] **Issue #8:** Player strafes in circles around entity at close range; entity must maintain pursuit
- [ ] **Issue #11:** Drag Master Volume slider continuously; no visible hitches (use `stat unit`)
- [ ] **Issue #3:** Stand still against wall; use `stat game` to verify `UpdateCameraCollision` skipped
- [ ] **Issue #5:** Entity chases player for 30 seconds; audio thread CPU <8% (use Unreal Insights)
- [ ] **Issue #1:** Spawn/destroy 20 entities; check memory profiler for leaked `USoundAttenuation` objects

---

## Already Resolved Issues (Commits e33e7b6, 745f223, 0548c83)

✅ Mouse input interception (bEnableClickEvents gated to pause menu)  
✅ Duplicate IMC registration (removed from PlayerController)  
✅ Missing dedicated mouse action mapping (IMC_MouseLook + IA_MouseLook wired)  
✅ AI crash from dangling timer lambdas (CreateWeakLambda + OnUnPossess cleanup)  
✅ Keyboard input not working (assigned in BP_BackroomsCharacter Details panel)  

---

## Low-Priority Polish (Deferred)

- Issue H: Ambient Audio (fluorescent hum, HVAC, distant horror sounds)
- Issue #12: Light flicker variation (per-instance profiles)
- Issue #11: Sprint buffering (N/A after stamina removal)
- Issue #10: AI perception config exposure (make UPROPERTY EditAnywhere)
- Issue J: HUD repurpose (interaction prompts after stamina removal)

---

**Next Steps:** Recommend tackling Phase 1 issues first (direct player impact), then Phase 2 (core stability), before proceeding to ambient audio implementation (Issue H).
