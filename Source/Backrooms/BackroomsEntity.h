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
class UFootstepDataAsset;
class UPointLightComponent;
class UAnimSequence;
class ABackroomsCharacter;

USTRUCT()
struct FBackroomsTrackedLight
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TWeakObjectPtr<UPointLightComponent> LightComponent;

	UPROPERTY(Transient)
	float BaseIntensity = 0.0f;

	UPROPERTY(Transient)
	FVector Location = FVector::ZeroVector;
};

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

	/** Footstep data asset grouping footstep sounds and physical material */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Footsteps")
	TObjectPtr<UFootstepDataAsset> FootstepData;

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

	// --- Ceiling Light Distortion & Blackout Proximity System ---

	/** Radius (cm) within which ceiling lights begin flickering erratically (default: 14m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|LightDistortion", meta = (ClampMin = "300.0", ClampMax = "3000.0"))
	float LightFlickerRadius;

	/** Radius (cm) within which ceiling lights directly fail / blackout (default: 5m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|LightDistortion", meta = (ClampMin = "100.0", ClampMax = "1000.0"))
	float LightBlackoutRadius;

	/** Whether entity distorts and shuts off nearby lights */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|LightDistortion")
	bool bEnableLightDistortion;

	UFUNCTION(BlueprintCallable, Category = "AI|LightDistortion")
	void SetLightDistortionEnabled(bool bEnabled) { bEnableLightDistortion = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "AI|LightDistortion")
	bool IsLightDistortionEnabled() const { return bEnableLightDistortion; }

	// --- Horror Attack & Jumpscare Parameters ---

	/** Maximum distance (cm) to target player to initiate aggressive attack / jumpscare strike (default: 140cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Attack", meta = (ClampMin = "50.0", ClampMax = "300.0"))
	float AttackRange;

	/** Cooldown (seconds) between attack attempts (default: 3.5s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Attack")
	float AttackCooldown;

	/** Loud jumpscare scream sound played during attack strike */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Attack")
	TObjectPtr<USoundBase> AttackScreamSound;

	/** Heavy horror impact hit sound played during attack strike */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Attack")
	TObjectPtr<USoundBase> AttackHitSound;

	/** Attack animation sequence played on skeletal mesh during strike */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Attack")
	TObjectPtr<UAnimSequence> AttackAnimation;

	/** Whether entity is currently executing an attack animation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Attack")
	bool bIsAttacking;

	/** Triggers aggressive lunging attack, jumpscare audio, animation, and player catch sequence */
	UFUNCTION(BlueprintCallable, Category = "AI|Attack")
	bool ExecuteAttack(ABackroomsCharacter* TargetPlayerChar);

	/** Applies pitch-black void shadow material overrides to all skeletal mesh slots to obscure features */
	UFUNCTION(BlueprintCallable, Category = "AI|Visual")
	void ApplyShadowVoidMaterial();

	/** Dynamic material instances created from M_EntityShadowVoid */
	UPROPERTY(Transient)
	TArray<TObjectPtr<class UMaterialInstanceDynamic>> DynamicMaterials;

	/** Dark obsidian silhouette color when illuminated by active ceiling light */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Visual")
	FLinearColor LitSilhouetteColor;

	/** Pure black void color when light is off or in unlit areas */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Visual")
	FLinearColor UnlitSilhouetteColor;

	/** Roughness when illuminated by light (matte obsidian) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Visual")
	float LitRoughness;

	/** Roughness when unlit (1.0 = zero specular reflection) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Visual")
	float UnlitRoughness;

	/** Specular when illuminated by light (0.08) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Visual")
	float LitSpecular;

	/** Specular when unlit (0.0 = zero highlight) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Visual")
	float UnlitSpecular;

	/** Whether entity is currently under an active, turned-on ceiling light */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Visual")
	bool bIsIlluminatedByLight;

	/** Sets stalker visibility and collision during stalking disappearances */
	UFUNCTION(BlueprintCallable, Category = "AI|Visual")
	void SetStalkerVisibility(bool bVisible);

	/** Returns whether the entity is currently illuminated by active light */
	UFUNCTION(BlueprintPure, Category = "AI|Visual")
	bool IsUnderActiveLight() const { return bIsIlluminatedByLight; }

	/** Event triggered when a footstep is played (for BP extension / VFX) */
	UFUNCTION(BlueprintImplementableEvent, Category = "AI|Footsteps")
	void OnFootstepPlayed(USoundBase* Sound, FVector Location, float Volume, float Pitch, bool bIsCarpet, EBackroomsAIState State);

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Accumulates distance and triggers footsteps at appropriate cadence */
	void UpdateFootsteps(float DeltaTime);

	/** Executes downward trace, physical material check, and spatialized playback */
	void TriggerFootstep(EBackroomsAIState State);

	/** Discovers all point lights in the level and records their baseline intensity */
	void InitializeTrackedLights();

	/** Modulates nearby point light intensities based on distance to entity */
	void UpdateLightDistortion(float DeltaTime);

	/** Updates dynamic lighting silhouette shading based on local ceiling light illumination */
	void UpdateLightingSilhouette(float DeltaTime);

	UPROPERTY(Transient)
	FLinearColor CurrentSilhouetteColor;

	UPROPERTY(Transient)
	float CurrentRoughness;

	UPROPERTY(Transient)
	float CurrentSpecular;

	UPROPERTY(Transient)
	TArray<FBackroomsTrackedLight> TrackedLights;

	/** Distance moved since last footstep */
	float FootstepDistanceAccumulator;

	/** Indices for non-repeating shuffle */
	int32 LastCarpetSoundIndex;
	int32 LastHeavySoundIndex;

	/** Pre-allocated audio component pool for footsteps to avoid spawning overhead and concurrency spikes */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> FootstepAudioPool;

	int32 NextFootstepPoolIndex;
	float LastFootstepTime;
	float AttackCooldownTimer;

	/** Setup attenuation settings programmatically */
	void SetupAttenuationSettings();
};
