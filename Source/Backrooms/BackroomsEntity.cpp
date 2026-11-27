// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsEntity.h"
#include "BackroomsAssetManager.h"
#include "BackroomsFootstepDataAsset.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

ABackroomsEntity::ABackroomsEntity()
	: WanderStepDistance(175.0f)
	, SearchStepDistance(145.0f)
	, ChaseStepDistance(125.0f)
	, WanderFootstepVolume(0.55f)
	, SearchFootstepVolume(0.72f)
	, ChaseFootstepVolume(1.05f)
	, CachedAIState(EBackroomsAIState::IdleWander)
	, FootstepDistanceAccumulator(0.0f)
	, LastCarpetSoundIndex(-1)
	, LastHeavySoundIndex(-1)
	, NextFootstepPoolIndex(0)
	, LastFootstepTime(0.0f)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create ambient breathing audio component
	BreathingAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BreathingAudioComponent"));
	BreathingAudioComponent->SetupAttachment(RootComponent);
	BreathingAudioComponent->bAutoActivate = true;
	BreathingAudioComponent->bStopWhenOwnerDestroyed = true;

	// Configure inline 3D attenuation for breathing so it spatializes naturally
	BreathingAudioComponent->bOverrideAttenuation = true;
	FSoundAttenuationSettings& BreathingAtten = BreathingAudioComponent->AttenuationOverrides;
	BreathingAtten.bAttenuate = true;
	BreathingAtten.bSpatialize = true;
	BreathingAtten.SpatializationAlgorithm = SPATIALIZATION_Default;
	BreathingAtten.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	BreathingAtten.FalloffDistance = 2500.0f; // Audible within 25 meters
	BreathingAtten.bAttenuateWithLPF = true;
	BreathingAtten.LPFRadiusMin = 400.0f;
	BreathingAtten.LPFRadiusMax = 2500.0f;
	BreathingAtten.LPFFrequencyAtMin = 20000.0f;
	BreathingAtten.LPFFrequencyAtMax = 1000.0f;
	BreathingAtten.bEnableOcclusion = true;
	BreathingAtten.OcclusionTraceChannel = ECC_Visibility;
	BreathingAtten.OcclusionVolumeAttenuation = 0.35f;

	FootstepData = nullptr;
}

void ABackroomsEntity::SetupAttenuationSettings()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	// Only create if it doesn't exist
	if (!FootstepAttenuation)
	{
		FootstepAttenuation = NewObject<USoundAttenuation>(
			this,
			USoundAttenuation::StaticClass(),
			TEXT("EntityFootstepAttenuation"),
			RF_Transient
		);

		if (FootstepAttenuation)
		{
			FSoundAttenuationSettings& Atten = FootstepAttenuation->Attenuation;
			Atten.bAttenuate = true;
			Atten.bSpatialize = true;
			Atten.SpatializationAlgorithm = SPATIALIZATION_Default;
			Atten.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
			Atten.AttenuationShape = EAttenuationShape::Sphere;
			Atten.FalloffDistance = 3500.0f; // 35 meters: player hears distant approaching corridor steps
			Atten.bAttenuateWithLPF = true;
			Atten.LPFRadiusMin = 600.0f;
			Atten.LPFRadiusMax = 3500.0f;
			Atten.LPFFrequencyAtMin = 20000.0f;
			Atten.LPFFrequencyAtMax = 800.0f;
			Atten.bEnableOcclusion = true;
			Atten.OcclusionTraceChannel = ECC_Visibility;
			Atten.OcclusionLowPassFilterFrequency = 350.0f;
			Atten.OcclusionVolumeAttenuation = 0.40f;
			Atten.OcclusionInterpolationTime = 0.15f;
		}
	}
}

void ABackroomsEntity::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Skip asset loading during Class Default Object (CDO) construction to prevent startup blocking
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	SetupAttenuationSettings();

	// Populate from primary footstep data asset if assigned
	if (FootstepData)
	{
		if (!CarpetPhysicalMaterial && FootstepData->CarpetPhysicalMaterial)
		{
			CarpetPhysicalMaterial = FootstepData->CarpetPhysicalMaterial;
		}
		if (CarpetFootstepSounds.Num() == 0)
		{
			CarpetFootstepSounds = FootstepData->CarpetFootstepSounds;
		}
		if (!DefaultFootstepSound && FootstepData->DefaultFootstepSounds.Num() > 0)
		{
			DefaultFootstepSound = FootstepData->DefaultFootstepSounds[0];
		}
		if (HeavyFootstepSounds.Num() == 0)
		{
			HeavyFootstepSounds = FootstepData->HeavyFootstepSounds;
		}
	}

	// Fallback lazy load assets on spawned instances from Asset Manager
	UBackroomsAssetManager& AssetManager = UBackroomsAssetManager::Get();

	if (!CarpetPhysicalMaterial)
	{
		CarpetPhysicalMaterial = AssetManager.CarpetPhysicalMaterial.LoadSynchronous();
	}
	if (CarpetFootstepSounds.Num() == 0)
	{
		for (const TSoftObjectPtr<USoundBase>& SoftSnd : AssetManager.CarpetFootstepSounds)
		{
			if (USoundBase* Snd = SoftSnd.LoadSynchronous())
			{
				CarpetFootstepSounds.Add(Snd);
			}
		}
	}
	if (HeavyFootstepSounds.Num() == 0)
	{
		for (const TSoftObjectPtr<USoundBase>& SoftSnd : AssetManager.HeavyFootstepSounds)
		{
			if (USoundBase* Snd = SoftSnd.LoadSynchronous())
			{
				HeavyFootstepSounds.Add(Snd);
			}
		}
	}
	if (!DefaultFootstepSound)
	{
		DefaultFootstepSound = AssetManager.ConcreteFootstepSound.LoadSynchronous();
	}
	if (!BreathingSound)
	{
		BreathingSound = AssetManager.SicklyBreathingSound.LoadSynchronous();
	}
	if (!GrowlSound)
	{
		GrowlSound = AssetManager.ZombieGrowlSound.LoadSynchronous();
	}

	// Initialize pre-allocated audio component pool for footsteps
	if (FootstepAudioPool.Num() == 0)
	{
		for (int32 i = 0; i < 3; ++i)
		{
			UAudioComponent* PoolComp = NewObject<UAudioComponent>(this, *FString::Printf(TEXT("FootstepAudioComp_%d"), i));
			if (PoolComp)
			{
				PoolComp->bAutoActivate = false;
				PoolComp->SetupAttachment(RootComponent);
				PoolComp->RegisterComponent();
				if (FootstepAttenuation)
				{
					PoolComp->AttenuationSettings = FootstepAttenuation;
				}
				FootstepAudioPool.Add(PoolComp);
			}
		}
	}
}

void ABackroomsEntity::BeginPlay()
{
	Super::BeginPlay();

	// Defer audio playback to next tick so Blueprint overrides have fully initialized all property references
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick([this]()
		{
			if (BreathingAudioComponent && BreathingSound && !IsActorBeingDestroyed())
			{
				BreathingAudioComponent->SetSound(BreathingSound);
				BreathingAudioComponent->SetVolumeMultiplier(0.35f);
				BreathingAudioComponent->Play();
			}
		});
	}

	UE_LOG(LogTemp, Warning, TEXT("[ABackroomsEntity] Initialized entity at %s with %d carpet and %d heavy footstep samples."),
		*GetActorLocation().ToString(), CarpetFootstepSounds.Num(), HeavyFootstepSounds.Num());
}

void ABackroomsEntity::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update cached AI state from controller
	if (ABackroomsAIController* AICon = Cast<ABackroomsAIController>(GetController()))
	{
		CachedAIState = AICon->CurrentState;
	}

	// Modulate ambient breathing volume based on state
	if (BreathingAudioComponent && BreathingAudioComponent->IsPlaying())
	{
		float TargetVolume = 0.35f;
		switch (CachedAIState)
		{
		case EBackroomsAIState::IdleWander:
			TargetVolume = 0.35f;
			break;
		case EBackroomsAIState::Search:
			TargetVolume = 0.55f;
			break;
		case EBackroomsAIState::Chase:
			TargetVolume = 0.85f;
			break;
		}
		BreathingAudioComponent->SetVolumeMultiplier(FMath::FInterpTo(BreathingAudioComponent->VolumeMultiplier, TargetVolume, DeltaTime, 3.0f));
	}

	UpdateFootsteps(DeltaTime);
}

void ABackroomsEntity::UpdateFootsteps(float DeltaTime)
{
	if (!GetCharacterMovement() || !GetCharacterMovement()->IsMovingOnGround())
	{
		FootstepDistanceAccumulator = 0.0f;
		return;
	}

	const float Speed2D = GetVelocity().Size2D();
	if (Speed2D < 15.0f)
	{
		FootstepDistanceAccumulator = 0.0f;
		return;
	}

	FootstepDistanceAccumulator += Speed2D * DeltaTime;

	// Dynamic stride distance based on operational AI State
	float CurrentStepDistance = WanderStepDistance;
	switch (CachedAIState)
	{
	case EBackroomsAIState::IdleWander:
		CurrentStepDistance = WanderStepDistance; // ~175cm -> ~1s per step at 180cm/s (slow, rhythmic)
		break;
	case EBackroomsAIState::Search:
		CurrentStepDistance = SearchStepDistance; // ~145cm -> ~0.6s per step at 240cm/s (deliberate stalking)
		break;
	case EBackroomsAIState::Chase:
		CurrentStepDistance = ChaseStepDistance;  // ~125cm -> ~0.28s per step at 450cm/s (rapid heavy pursuit)
		break;
	}

	// Clamp minimum inter-step interval to 0.15s (max 6.7 steps/sec)
	if (UWorld* World = GetWorld())
	{
		if (World->GetTimeSeconds() - LastFootstepTime < 0.15f)
		{
			return;
		}
	}

	if (FootstepDistanceAccumulator >= CurrentStepDistance)
	{
		FootstepDistanceAccumulator = 0.0f;
		if (UWorld* World = GetWorld())
		{
			LastFootstepTime = World->GetTimeSeconds();
		}
		TriggerFootstep(CachedAIState);
	}
}

void ABackroomsEntity::TriggerFootstep(EBackroomsAIState State)
{
	const float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.0f;
	FVector ImpactPoint = GetActorLocation() - FVector(0.0f, 0.0f, CapsuleHalfHeight);
	bool bIsCarpet = true;

	if (State != EBackroomsAIState::Chase)
	{
		const FVector TraceStart = GetActorLocation();
		const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, CapsuleHalfHeight + 35.0f);

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EntityFootstepTrace), false, this);
		QueryParams.bReturnPhysicalMaterial = true;

		FHitResult HitResult;
		const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

		if (!bHit)
		{
			return;
		}

		ImpactPoint = HitResult.ImpactPoint;
		UPhysicalMaterial* HitPhysMat = HitResult.PhysMaterial.Get();
		bIsCarpet = (HitPhysMat != nullptr) && (
			HitPhysMat == CarpetPhysicalMaterial ||
			HitPhysMat->SurfaceType == SurfaceType1 ||
			HitPhysMat->GetName().Contains(TEXT("Carpet"))
		);
	}
	else
	{
		// Disable expensive occlusion line trace during chase to eliminate CPU spikes
		bIsCarpet = true;
	}

	// Choose candidate sound set
	USoundBase* SoundToPlay = nullptr;

	if (State == EBackroomsAIState::Chase)
	{
		// In chase, prioritize heavy monster steps or layer carpet with monster stomps
		if (HeavyFootstepSounds.Num() > 0 && FMath::RandBool())
		{
			int32 SelectedIndex = FMath::RandRange(0, HeavyFootstepSounds.Num() - 1);
			SoundToPlay = HeavyFootstepSounds[SelectedIndex];
		}
		else if (bIsCarpet && CarpetFootstepSounds.Num() > 0)
		{
			int32 SelectedIndex = 0;
			if (CarpetFootstepSounds.Num() > 1)
			{
				do
				{
					SelectedIndex = FMath::RandRange(0, CarpetFootstepSounds.Num() - 1);
				} while (SelectedIndex == LastCarpetSoundIndex);
			}
			LastCarpetSoundIndex = SelectedIndex;
			SoundToPlay = CarpetFootstepSounds[SelectedIndex];
		}
	}
	else
	{
		// In wander / search, use carpet footsteps if on carpet, otherwise heavy/default
		if (bIsCarpet && CarpetFootstepSounds.Num() > 0)
		{
			int32 SelectedIndex = 0;
			if (CarpetFootstepSounds.Num() > 1)
			{
				do
				{
					SelectedIndex = FMath::RandRange(0, CarpetFootstepSounds.Num() - 1);
				} while (SelectedIndex == LastCarpetSoundIndex);
			}
			LastCarpetSoundIndex = SelectedIndex;
			SoundToPlay = CarpetFootstepSounds[SelectedIndex];
		}
		else if (HeavyFootstepSounds.Num() > 0)
		{
			int32 SelectedIndex = 0;
			if (HeavyFootstepSounds.Num() > 1)
			{
				do
				{
					SelectedIndex = FMath::RandRange(0, HeavyFootstepSounds.Num() - 1);
				} while (SelectedIndex == LastHeavySoundIndex);
			}
			LastHeavySoundIndex = SelectedIndex;
			SoundToPlay = HeavyFootstepSounds[SelectedIndex];
		}
		else
		{
			SoundToPlay = DefaultFootstepSound;
		}
	}

	if (!SoundToPlay)
	{
		return;
	}

	// State-dependent volume and pitch
	float Volume = WanderFootstepVolume;
	float Pitch = 0.85f;

	switch (State)
	{
	case EBackroomsAIState::IdleWander:
		Volume = WanderFootstepVolume;
		Pitch = FMath::FRandRange(0.80f, 0.88f); // Deep, ominous, slow
		break;
	case EBackroomsAIState::Search:
		Volume = SearchFootstepVolume;
		Pitch = FMath::FRandRange(0.78f, 0.85f); // Tense, calculated stalking
		break;
	case EBackroomsAIState::Chase:
		Volume = ChaseFootstepVolume;
		Pitch = FMath::FRandRange(0.72f, 0.80f); // Aggressive, menacing impact
		break;
	}

	// Play spatialized 3D sound at floor impact point using audio pool
	UAudioComponent* AudioComp = nullptr;
	if (FootstepAudioPool.Num() > 0)
	{
		AudioComp = FootstepAudioPool[NextFootstepPoolIndex];
		NextFootstepPoolIndex = (NextFootstepPoolIndex + 1) % FootstepAudioPool.Num();
	}

	if (AudioComp)
	{
		AudioComp->SetWorldLocation(ImpactPoint);
		AudioComp->SetSound(SoundToPlay);
		AudioComp->SetVolumeMultiplier(Volume);
		AudioComp->SetPitchMultiplier(Pitch);
		if (FootstepAttenuation)
		{
			AudioComp->AttenuationSettings = FootstepAttenuation;
		}
		AudioComp->Play();
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			SoundToPlay,
			ImpactPoint,
			FRotator::ZeroRotator,
			Volume,
			Pitch,
			0.0f,
			FootstepAttenuation,
			nullptr,
			this
		);
	}

	OnFootstepPlayed(SoundToPlay, ImpactPoint, Volume, Pitch, bIsCarpet, State);
}
