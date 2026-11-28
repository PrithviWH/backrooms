// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BackroomsAtmosphereManager.generated.h"

class UAudioComponent;
class USoundBase;
class USoundClass;
class USoundAttenuation;

UENUM(BlueprintType)
enum class EBackroomsAtmosphereEventType : uint8
{
	DoorSlam UMETA(DisplayName = "Distant Door Slam"),
	DoorKnock UMETA(DisplayName = "Creepy Door Knock"),
	DistantThud UMETA(DisplayName = "Distant Quiet Thud"),
	HeavyFalling UMETA(DisplayName = "Heavy Falling Object"),
	ReverbImpact UMETA(DisplayName = "Cinematic Reverb Impact"),
	WhisperSense UMETA(DisplayName = "Demonic Whisper I Sense You"),
	PossessedLaugh UMETA(DisplayName = "Possessed Eerie Laughter"),
	ClockTicking UMETA(DisplayName = "Ominous Tension Clock"),
	WhisperOverHere UMETA(DisplayName = "Whisper Over Here"),
	WhisperDontLookBack UMETA(DisplayName = "Whisper Dont Look Back"),
	WhisperNoEscape UMETA(DisplayName = "Whisper No Escape"),
	WhisperCounting UMETA(DisplayName = "Whisper Counting"),
	WhisperWhyRunning UMETA(DisplayName = "Whisper Why Running"),
	SubtlePipeDrip UMETA(DisplayName = "Subtle Pipe Drip"),
	SubtleWallGroan UMETA(DisplayName = "Subtle Wall Groan"),
	SubtleVentDraft UMETA(DisplayName = "Subtle Vent Draft"),
	SubtleLightArc UMETA(DisplayName = "Subtle Light Arc"),
	SubtleCeilingScuttle UMETA(DisplayName = "Subtle Ceiling Scuttle")
};

/**
 * ABackroomsAtmosphereManager
 *
 * Drives continuous psychological horror atmosphere, low-frequency suspense drones,
 * and contextual 3D location-based scary sound events when the player navigates,
 * reaches dead ends, turns blind corners, or freezes in fear.
 *
 * Audio Class Routing: Routes strictly through SC_Ambience (<= 0.5 volume) per AGENTS.md.
 */
UCLASS()
class BACKROOMS_API ABackroomsAtmosphereManager : public AActor
{
	GENERATED_BODY()

public:
	ABackroomsAtmosphereManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Trigger a specific 3D location horror sound effect */
	UFUNCTION(BlueprintCallable, Category = "Atmosphere")
	void TriggerHorrorCueAtLocation(const FVector& Location, EBackroomsAtmosphereEventType EventType);

	/** Trigger a contextual horror cue around the player based on their immediate navigation and surroundings */
	UFUNCTION(BlueprintCallable, Category = "Atmosphere")
	void TriggerContextualHorrorCue();

	/** Set whether dynamic scary events are enabled */
	UFUNCTION(BlueprintCallable, Category = "Atmosphere")
	void SetDynamicEventsEnabled(bool bEnabled) { bDynamicEventsEnabled = bEnabled; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Continuous psychological dread background drone */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere|Components")
	TObjectPtr<UAudioComponent> DroneAudioComponent;

	/** Continuous sub-bass industrial room tone layer */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atmosphere|Components")
	TObjectPtr<UAudioComponent> RoomToneAudioComponent;

	/** Minimum seconds between dynamic location-based horror sounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Pacing")
	float MinEventCooldown;

	/** Maximum seconds between dynamic location-based horror sounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Pacing")
	float MaxEventCooldown;

	/** Volume of ambient suspense drone (strictly <= 0.5 per AGENTS.md) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Audio", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float DroneVolume;

	/** Volume of industrial room tone (strictly <= 0.5 per AGENTS.md) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Audio", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float RoomToneVolume;

	/** Whether dynamic scare cues trigger during exploration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Pacing")
	bool bDynamicEventsEnabled;

private:
	float EventCooldownTimer;
	float TimeStillInOnePlace;
	float AccumulatedDistanceWalked;
	FVector LastPlayerLocation;

	UPROPERTY()
	TObjectPtr<USoundClass> CachedAmbienceClass;

	UPROPERTY()
	TObjectPtr<USoundAttenuation> SpatialAttenuation;

	void UpdateHorrorDirector(float DeltaTime);
	FVector FindCreepySoundLocation(const AActor* PlayerActor, bool bBehindPlayer = true) const;
	USoundBase* GetSoundForEventType(EBackroomsAtmosphereEventType EventType) const;
};
