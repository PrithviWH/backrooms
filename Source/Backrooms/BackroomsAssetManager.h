// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "BackroomsAssetManager.generated.h"

class UPhysicalMaterial;
class USoundBase;
class USoundMix;
class USoundClass;
class UInputMappingContext;
class UInputAction;
class UAnimSequence;

/**
 * Central asset manager for Backrooms 5.8.
 * Provides soft object paths and async loading for all primary project assets,
 * replacing hardcoded asset strings and enabling runtime validation via AssetRegistry.
 */
UCLASS()
class BACKROOMS_API UBackroomsAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	UBackroomsAssetManager();

	/** Static accessor to project asset manager singleton or CDO fallback */
	static UBackroomsAssetManager& Get();

	virtual void StartInitialLoading() override;

	// --- Physical Materials ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Physics")
	TSoftObjectPtr<UPhysicalMaterial> CarpetPhysicalMaterial;

	// --- Footstep & AI Sounds ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TArray<TSoftObjectPtr<USoundBase>> CarpetFootstepSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TArray<TSoftObjectPtr<USoundBase>> HeavyFootstepSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> ConcreteFootstepSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> SicklyBreathingSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> ZombieGrowlSound;

	// --- Heartbeat & Horror Sounds ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> HeartbeatSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> OutOfBreathSound;

	/** Dramatic string tension stinger played when the player first sees the entity in front of them */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> EntitySightStingerSound;

	// --- Ambient Atmosphere Drones & Musics ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> AtmosphereSuspenseDrone;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> AtmosphereTrailerDrone;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> AtmosphereRoomTone;

	// --- Dynamic Location Horror SFX ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> CreepyDoorSlamSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> CreepyDoorKnockSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> CreepyDistantThudSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> CreepyHeavyFallingSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> CreepyReverbImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> CreepyWhisperSenseSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> CreepyPossessedLaughSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> CreepyClockTickSound;

	// --- 5 Creepy Whispered Voices ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio|Voices")
	TSoftObjectPtr<USoundBase> VoiceWhisperOverHere;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio|Voices")
	TSoftObjectPtr<USoundBase> VoiceWhisperDontLookBack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio|Voices")
	TSoftObjectPtr<USoundBase> VoiceWhisperNoEscape;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio|Voices")
	TSoftObjectPtr<USoundBase> VoiceWhisperCounting;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio|Voices")
	TSoftObjectPtr<USoundBase> VoiceWhisperWhyRunning;

	// --- Subtle Environmental Horror SFX ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio|SubtleSFX")
	TSoftObjectPtr<USoundBase> SFXSubtlePipeDrip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio|SubtleSFX")
	TSoftObjectPtr<USoundBase> SFXSubtleWallGroan;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio|SubtleSFX")
	TSoftObjectPtr<USoundBase> SFXSubtleVentDraft;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio|SubtleSFX")
	TSoftObjectPtr<USoundBase> SFXSubtleLightArc;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio|SubtleSFX")
	TSoftObjectPtr<USoundBase> SFXSubtleCeilingScuttle;

	// --- Jumpscare & Attack Assets ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> JumpscareScreamSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> JumpscareHitSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> JumpscareImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Animation")
	TSoftObjectPtr<UAnimSequence> MonsterAttackAnimation;

	// --- Input Assets ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputMappingContext> MouseLookMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> PauseAction;

	// --- Audio Mix & Sound Classes ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|AudioMix")
	TSoftObjectPtr<USoundMix> BackroomsSoundMix;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|AudioMix")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|AudioMix")
	TSoftObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|AudioMix")
	TSoftObjectPtr<USoundClass> AmbienceSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|AudioMix")
	TSoftObjectPtr<USoundClass> EntitySoundClass;

	/** Validates that configured soft paths point to existing assets in the AssetRegistry */
	UFUNCTION(BlueprintCallable, Category = "Assets")
	bool ValidateAssetReferences(TArray<FString>& OutMissingAssets);

	/** Preloads all critical soft assets asynchronously */
	TSharedPtr<FStreamableHandle> PreloadCriticalAssetsAsync(FStreamableDelegate OnComplete = FStreamableDelegate());
};
