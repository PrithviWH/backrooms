// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BackroomsAIController.h"
#include "BackroomsEntity.generated.h"

class UAudioComponent;
class USoundBase;
class USoundAttenuation;
class UPhysicalMaterial;

/**
 * Authoritative Entity / Monster Character for Backrooms.
 * Implements AI movement cadence, dynamic footstep audio that varies with AI state
 * (slow/deliberate wander, cautious search, rapid aggressive chase),
 * spatialized 3D sound attenuation with distance LPF and wall occlusion,
 * carpet physical material detection, and ambient sickly breathing audio.
 */
UCLASS()
class BACKROOMS_API ABackroomsEntity : public ACharacter
{
	GENERATED_BODY()

public:
	ABackroomsEntity();

	/** Ambient attached audio component for sickly stalking breathing */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Audio")
	TObjectPtr<UAudioComponent> BreathingAudioComponent;

	/** Carpet footstep sounds (soft Berber carpet thuds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	TArray<TObjectPtr<USoundBase>> CarpetFootstepSounds;

	/** Heavy monster footstep sounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	TArray<TObjectPtr<USoundBase>> HeavyFootstepSounds;

	/** Default fallback footstep sound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	TObjectPtr<USoundBase> DefaultFootstepSound;

	/** Stomp sound played during aggressive chase */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	TObjectPtr<USoundBase> ChaseStompSound;

	/** Growl sound played occasionally when stalking or chasing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Audio")
	TObjectPtr<USoundBase> GrowlSound;

	/** Sickly breathing sound asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Audio")
	TObjectPtr<USoundBase> BreathingSound;

	/** Carpet physical material reference for detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	TObjectPtr<UPhysicalMaterial> CarpetPhysicalMaterial;

	/** 3D sound attenuation for spatialized footsteps */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	TObjectPtr<USoundAttenuation> FootstepAttenuation;

	/** Step distance while wandering (slow cadence: ~175cm -> ~1s interval at 180cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	float WanderStepDistance;

	/** Step distance while searching (deliberate cadence: ~145cm -> ~0.6s interval at 240cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	float SearchStepDistance;

	/** Step distance while chasing (rapid cadence: ~125cm -> ~0.28s interval at 450cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	float ChaseStepDistance;

	/** Footstep volume while wandering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	float WanderFootstepVolume;

	/** Footstep volume while searching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	float SearchFootstepVolume;

	/** Footstep volume while chasing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Footsteps")
	float ChaseFootstepVolume;

	/** Current operational AI state cached from controller */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|State")
	EBackroomsAIState CachedAIState;

	/** Event triggered when a footstep is played (for BP extension / VFX) */
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|Footsteps")
	void OnFootstepPlayed(USoundBase* Sound, FVector Location, float Volume, float Pitch, bool bIsCarpet, EBackroomsAIState State);

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;

	/** Accumulates distance and triggers footsteps at appropriate cadence */
	void UpdateFootsteps(float DeltaTime);

	/** Executes downward trace, physical material check, and spatialized playback */
	void TriggerFootstep(EBackroomsAIState State);

	/** Distance moved since last footstep */
	float FootstepDistanceAccumulator;

	/** Indices for non-repeating shuffle */
	int32 LastCarpetSoundIndex;
	int32 LastHeavySoundIndex;

	/** Setup attenuation settings programmatically */
	void SetupAttenuationSettings();
};
