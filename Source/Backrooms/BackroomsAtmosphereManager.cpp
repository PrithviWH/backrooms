// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsAtmosphereManager.h"
#include "BackroomsAssetManager.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"

ABackroomsAtmosphereManager::ABackroomsAtmosphereManager()
	: MinEventCooldown(22.0f)
	, MaxEventCooldown(45.0f)
	, DroneVolume(0.32f)
	, RoomToneVolume(0.20f)
	, bDynamicEventsEnabled(true)
	, EventCooldownTimer(12.0f) // First cue plays after 12s of roaming
	, TimeStillInOnePlace(0.0f)
	, AccumulatedDistanceWalked(0.0f)
	, LastPlayerLocation(FVector::ZeroVector)
	, CachedAmbienceClass(nullptr)
	, SpatialAttenuation(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	DroneAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("DroneAudioComponent"));
	DroneAudioComponent->SetupAttachment(RootComponent);
	DroneAudioComponent->bAutoActivate = false;
	DroneAudioComponent->bIsUISound = false;

	RoomToneAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("RoomToneAudioComponent"));
	RoomToneAudioComponent->SetupAttachment(RootComponent);
	RoomToneAudioComponent->bAutoActivate = false;
	RoomToneAudioComponent->bIsUISound = false;
}

void ABackroomsAtmosphereManager::BeginPlay()
{
	Super::BeginPlay();

	UBackroomsAssetManager& AssetManager = UBackroomsAssetManager::Get();

	// Cache Ambience Sound Class for proper audio mixing (<= 0.5 volume per AGENTS.md)
	CachedAmbienceClass = AssetManager.AmbienceSoundClass.LoadSynchronous();

	// Initialize background drone
	USoundBase* DroneSound = AssetManager.AtmosphereSuspenseDrone.LoadSynchronous();
	if (!DroneSound)
	{
		DroneSound = AssetManager.AtmosphereTrailerDrone.LoadSynchronous();
	}

	if (DroneAudioComponent && DroneSound)
	{
		DroneAudioComponent->SetSound(DroneSound);
		if (CachedAmbienceClass)
		{
			DroneAudioComponent->SoundClassOverride = CachedAmbienceClass;
		}
		DroneAudioComponent->SetVolumeMultiplier(FMath::Clamp(DroneVolume, 0.0f, 0.5f));
		DroneAudioComponent->Play();
		UE_LOG(LogTemp, Log, TEXT("[AtmosphereManager] Background psychological drone active: %s"), *DroneSound->GetName());
	}

	// Initialize deep room tone
	USoundBase* RoomToneSound = AssetManager.AtmosphereRoomTone.LoadSynchronous();
	if (RoomToneAudioComponent && RoomToneSound)
	{
		RoomToneAudioComponent->SetSound(RoomToneSound);
		if (CachedAmbienceClass)
		{
			RoomToneAudioComponent->SoundClassOverride = CachedAmbienceClass;
		}
		RoomToneAudioComponent->SetVolumeMultiplier(FMath::Clamp(RoomToneVolume, 0.0f, 0.5f));
		RoomToneAudioComponent->Play();
		UE_LOG(LogTemp, Log, TEXT("[AtmosphereManager] Sub-bass room tone active: %s"), *RoomToneSound->GetName());
	}

	// Create spatial attenuation object for dynamic 3D location horror cues
	SpatialAttenuation = NewObject<USoundAttenuation>(this, TEXT("AtmosphereSpatialAttenuation"));
	if (SpatialAttenuation)
	{
		FSoundAttenuationSettings& Settings = SpatialAttenuation->Attenuation;
		Settings.bAttenuate = true;
		Settings.bSpatialize = true;
		Settings.SpatializationAlgorithm = ESoundSpatializationAlgorithm::SPATIALIZATION_Default;
		Settings.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
		Settings.AttenuationShape = EAttenuationShape::Sphere;
		Settings.FalloffDistance = 3500.0f;
		Settings.bAttenuateWithLPF = true;
		Settings.LPFRadiusMin = 800.0f;
		Settings.LPFRadiusMax = 3500.0f;
	}

	// Initialize player position tracking
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		LastPlayerLocation = PlayerPawn->GetActorLocation();
	}
}

void ABackroomsAtmosphereManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDynamicEventsEnabled)
	{
		UpdateHorrorDirector(DeltaTime);
	}
}

void ABackroomsAtmosphereManager::UpdateHorrorDirector(float DeltaTime)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	const FVector CurrentLocation = PlayerPawn->GetActorLocation();
	const float DistMoved = FVector::Dist2D(CurrentLocation, LastPlayerLocation);

	if (DistMoved < 15.0f)
	{
		// Player is standing still or lurking in place
		TimeStillInOnePlace += DeltaTime;
	}
	else
	{
		// Player is actively navigating the backrooms
		TimeStillInOnePlace = 0.0f;
		AccumulatedDistanceWalked += DistMoved;
		LastPlayerLocation = CurrentLocation;
	}

	EventCooldownTimer -= DeltaTime;

	if (EventCooldownTimer <= 0.0f)
	{
		// Check whether conditions warrant a scary location cue
		const bool bPlayerExploredFar = (AccumulatedDistanceWalked >= 1400.0f);
		const bool bPlayerFrozenInFear = (TimeStillInOnePlace >= 16.0f);

		if (bPlayerExploredFar || bPlayerFrozenInFear)
		{
			TriggerContextualHorrorCue();
			EventCooldownTimer = FMath::RandRange(MinEventCooldown, MaxEventCooldown);
			AccumulatedDistanceWalked = 0.0f;
			TimeStillInOnePlace = 0.0f;
		}
	}
}

void ABackroomsAtmosphereManager::TriggerContextualHorrorCue()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	EBackroomsAtmosphereEventType SelectedEvent = EBackroomsAtmosphereEventType::DistantThud;
	bool bSpawnBehind = true;

	if (TimeStillInOnePlace >= 14.0f)
	{
		// Player staying still: tension builds; clock ticking, uncanny counting whisper, ceiling scuttle, or laughter
		const int32 StillChoice = FMath::RandRange(0, 3);
		switch (StillChoice)
		{
		case 0:
			SelectedEvent = EBackroomsAtmosphereEventType::ClockTicking;
			bSpawnBehind = false;
			break;
		case 1:
			SelectedEvent = EBackroomsAtmosphereEventType::WhisperCounting;
			bSpawnBehind = true;
			break;
		case 2:
			SelectedEvent = EBackroomsAtmosphereEventType::SubtleCeilingScuttle;
			bSpawnBehind = false;
			break;
		default:
			SelectedEvent = EBackroomsAtmosphereEventType::PossessedLaugh;
			bSpawnBehind = true;
			break;
		}
	}
	else
	{
		// Actively walking: check forward line trace for dead end or wall in front
		FHitResult HitResult;
		const FVector TraceStart = PlayerPawn->GetActorLocation();
		const FVector TraceEnd = TraceStart + (PlayerPawn->GetActorForwardVector() * 400.0f);

		FCollisionQueryParams QueryParams(TEXT("AtmosphereDeadEndCheck"), false, PlayerPawn);
		const bool bWallInFront = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams);

		if (bWallInFront)
		{
			// Dead end / corner: creepy knocking, structural wall groan, or whispered warning from the other side!
			const int32 RandChoice = FMath::RandRange(0, 3);
			switch (RandChoice)
			{
			case 0:
				SelectedEvent = EBackroomsAtmosphereEventType::DoorKnock;
				bSpawnBehind = false;
				break;
			case 1:
				SelectedEvent = EBackroomsAtmosphereEventType::SubtleWallGroan;
				bSpawnBehind = false;
				break;
			case 2:
				SelectedEvent = EBackroomsAtmosphereEventType::WhisperDontLookBack;
				bSpawnBehind = true;
				break;
			default:
				SelectedEvent = EBackroomsAtmosphereEventType::DistantThud;
				bSpawnBehind = false;
				break;
			}
		}
		else
		{
			// Corridors & open halls: distant slams, eerie whispers, pipe drips, vent drafts, or light arc snaps
			const int32 Choice = FMath::RandRange(0, 7);
			switch (Choice)
			{
			case 0:
				SelectedEvent = EBackroomsAtmosphereEventType::WhisperOverHere;
				bSpawnBehind = (FMath::RandBool());
				break;
			case 1:
				SelectedEvent = EBackroomsAtmosphereEventType::WhisperWhyRunning;
				bSpawnBehind = true;
				break;
			case 2:
				SelectedEvent = EBackroomsAtmosphereEventType::SubtlePipeDrip;
				bSpawnBehind = false;
				break;
			case 3:
				SelectedEvent = EBackroomsAtmosphereEventType::SubtleVentDraft;
				bSpawnBehind = false;
				break;
			case 4:
				SelectedEvent = EBackroomsAtmosphereEventType::SubtleLightArc;
				bSpawnBehind = (FMath::RandBool());
				break;
			case 5:
				SelectedEvent = EBackroomsAtmosphereEventType::DoorSlam;
				bSpawnBehind = true;
				break;
			case 6:
				SelectedEvent = EBackroomsAtmosphereEventType::ReverbImpact;
				bSpawnBehind = false;
				break;
			default:
				SelectedEvent = EBackroomsAtmosphereEventType::WhisperNoEscape;
				bSpawnBehind = true;
				break;
			}
		}
	}

	const FVector SoundLocation = FindCreepySoundLocation(PlayerPawn, bSpawnBehind);
	TriggerHorrorCueAtLocation(SoundLocation, SelectedEvent);
}

FVector ABackroomsAtmosphereManager::FindCreepySoundLocation(const AActor* PlayerActor, bool bBehindPlayer) const
{
	if (!PlayerActor)
	{
		return GetActorLocation();
	}

	const FVector PlayerLoc = PlayerActor->GetActorLocation();
	const FVector Forward = PlayerActor->GetActorForwardVector();
	const FVector Right = PlayerActor->GetActorRightVector();

	const float Distance = FMath::RandRange(1400.0f, 2600.0f);
	const float LateralOffset = FMath::RandRange(-450.0f, 450.0f);

	FVector Direction = bBehindPlayer ? (-Forward) : Forward;
	FVector TargetLoc = PlayerLoc + (Direction * Distance) + (Right * LateralOffset);
	TargetLoc.Z = PlayerLoc.Z + 10.0f; // Keep at eye / room level

	// Line trace to ensure we don't spawn inside a solid wall
	FHitResult HitResult;
	FCollisionQueryParams Params(TEXT("AtmosphereSoundPlacement"), false, PlayerActor);
	if (GetWorld()->LineTraceSingleByChannel(HitResult, PlayerLoc, TargetLoc, ECC_WorldStatic, Params))
	{
		// Pull back 150cm from wall impact so sound resonates clearly in the open corridor
		TargetLoc = HitResult.Location - (Direction * 150.0f);
	}

	return TargetLoc;
}

USoundBase* ABackroomsAtmosphereManager::GetSoundForEventType(EBackroomsAtmosphereEventType EventType) const
{
	UBackroomsAssetManager& AssetManager = UBackroomsAssetManager::Get();

	switch (EventType)
	{
	case EBackroomsAtmosphereEventType::DoorSlam:
		return AssetManager.CreepyDoorSlamSound.LoadSynchronous();
	case EBackroomsAtmosphereEventType::DoorKnock:
		return AssetManager.CreepyDoorKnockSound.LoadSynchronous();
	case EBackroomsAtmosphereEventType::DistantThud:
		return AssetManager.CreepyDistantThudSound.LoadSynchronous();
	case EBackroomsAtmosphereEventType::HeavyFalling:
		return AssetManager.CreepyHeavyFallingSound.LoadSynchronous();
	case EBackroomsAtmosphereEventType::ReverbImpact:
		return AssetManager.CreepyReverbImpactSound.LoadSynchronous();
	case EBackroomsAtmosphereEventType::WhisperSense:
		return AssetManager.CreepyWhisperSenseSound.LoadSynchronous();
	case EBackroomsAtmosphereEventType::PossessedLaugh:
		return AssetManager.CreepyPossessedLaughSound.LoadSynchronous();
	case EBackroomsAtmosphereEventType::ClockTicking:
		return AssetManager.CreepyClockTickSound.LoadSynchronous();
	case EBackroomsAtmosphereEventType::WhisperOverHere:
		return AssetManager.VoiceWhisperOverHere.LoadSynchronous();
	case EBackroomsAtmosphereEventType::WhisperDontLookBack:
		return AssetManager.VoiceWhisperDontLookBack.LoadSynchronous();
	case EBackroomsAtmosphereEventType::WhisperNoEscape:
		return AssetManager.VoiceWhisperNoEscape.LoadSynchronous();
	case EBackroomsAtmosphereEventType::WhisperCounting:
		return AssetManager.VoiceWhisperCounting.LoadSynchronous();
	case EBackroomsAtmosphereEventType::WhisperWhyRunning:
		return AssetManager.VoiceWhisperWhyRunning.LoadSynchronous();
	case EBackroomsAtmosphereEventType::SubtlePipeDrip:
		return AssetManager.SFXSubtlePipeDrip.LoadSynchronous();
	case EBackroomsAtmosphereEventType::SubtleWallGroan:
		return AssetManager.SFXSubtleWallGroan.LoadSynchronous();
	case EBackroomsAtmosphereEventType::SubtleVentDraft:
		return AssetManager.SFXSubtleVentDraft.LoadSynchronous();
	case EBackroomsAtmosphereEventType::SubtleLightArc:
		return AssetManager.SFXSubtleLightArc.LoadSynchronous();
	case EBackroomsAtmosphereEventType::SubtleCeilingScuttle:
		return AssetManager.SFXSubtleCeilingScuttle.LoadSynchronous();
	default:
		return AssetManager.CreepyDistantThudSound.LoadSynchronous();
	}
}

void ABackroomsAtmosphereManager::TriggerHorrorCueAtLocation(const FVector& Location, EBackroomsAtmosphereEventType EventType)
{
	USoundBase* SoundToPlay = GetSoundForEventType(EventType);
	if (!SoundToPlay)
	{
		return;
	}

	// Slightly vary volume and pitch for organic psychological realism
	const float Volume = FMath::RandRange(0.32f, 0.44f); // Strictly <= 0.5 per AGENTS.md
	const float Pitch = FMath::RandRange(0.93f, 1.05f);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		SoundToPlay,
		Location,
		FRotator::ZeroRotator,
		Volume,
		Pitch,
		0.0f,
		SpatialAttenuation,
		nullptr,
		this
	);

	UE_LOG(LogTemp, Log, TEXT("[AtmosphereManager] Spatial horror cue triggered: Event=%d at (%s) Volume=%.2f Pitch=%.2f"),
		static_cast<int32>(EventType), *Location.ToCompactString(), Volume, Pitch);
}
