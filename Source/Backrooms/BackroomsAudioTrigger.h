// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BackroomsAudioTrigger.generated.h"

class USphereComponent;
class USoundBase;
class USoundAttenuation;
class USoundClass;

UENUM(BlueprintType)
enum class EBackroomsTriggerSoundCategory : uint8
{
	CreepyVoices UMETA(DisplayName = "Creepy Whispered Voices"),
	SubtleSFX    UMETA(DisplayName = "Subtle Environmental SFX"),
	MixedRandom  UMETA(DisplayName = "Random Voices & SFX"),
	SpecificSound UMETA(DisplayName = "Single Specific Sound")
};

/**
 * ABackroomsAudioTrigger
 *
 * Location-based 3D spatial audio trigger actor placed at corridors, dead ends,
 * rooms, and junctions. When the player enters the trigger radius, it plays
 * a spatialized 3D sound (subtle SFX or whispered voice) from a natural offset.
 *
 * Audio Class Routing: Routes strictly through SC_Ambience (<= 0.5 volume) per AGENTS.md.
 */
UCLASS()
class BACKROOMS_API ABackroomsAudioTrigger : public AActor
{
	GENERATED_BODY()

public:
	ABackroomsAudioTrigger();

	virtual void BeginPlay() override;

	/** Trigger sphere volume for player overlap */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger|Components")
	TObjectPtr<USphereComponent> TriggerVolume;

	/** Sound category to play on activation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Audio")
	EBackroomsTriggerSoundCategory SoundCategory;

	/** Specific sound to play when SoundCategory is SpecificSound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Audio")
	TSoftObjectPtr<USoundBase> SpecificSoundAsset;

	/** Minimum seconds between re-triggering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Pacing")
	float MinCooldown;

	/** Maximum seconds between re-triggering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Pacing")
	float MaxCooldown;

	/** Probability of triggering when entering the volume (0.0 to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Pacing", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TriggerChance;

	/** Volume multiplier (strictly <= 0.5 per AGENTS.md) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Audio", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float VolumeMultiplier;

	/** 3D spatial offset from trigger center (e.g. into ceiling vent +120cm Z or down hall) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Audio")
	FVector SpatialAudioOffset;

	/** Max 3D audible distance in cm (default 2600cm = 26m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Audio")
	float FalloffDistance;

	/** Whether this trigger fires only once per level session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Pacing")
	bool bOneShotOnly;

	/** Manually trigger the audio playback */
	UFUNCTION(BlueprintCallable, Category = "Trigger")
	void TriggerSound(AActor* InstigatorActor = nullptr);

protected:
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bHasFired;
	float CooldownTimer;
	FTimerHandle CooldownTimerHandle;

	UPROPERTY()
	TObjectPtr<USoundAttenuation> SpatialAttenuation;

	UPROPERTY()
	TObjectPtr<USoundClass> AmbienceSoundClass;

	void ResetCooldown();
	USoundBase* SelectRandomSound();
};
