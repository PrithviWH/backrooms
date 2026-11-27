// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsEntity.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

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

	// Load Physical Material: PM_Carpet
	static ConstructorHelpers::FObjectFinder<UPhysicalMaterial> CarpetMatFinder(
		TEXT("/Game/Backrooms/Materials/PM_Carpet.PM_Carpet"));
	if (CarpetMatFinder.Succeeded())
	{
		CarpetPhysicalMaterial = CarpetMatFinder.Object;
	}

	// Load Carpet Footsteps (Step1 - Step5)
	const TCHAR* CarpetAssetPaths[] = {
		TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step1.Step1"),
		TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step2.Step2"),
		TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step3.Step3"),
		TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step4.Step4"),
		TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step5.Step5")
	};
	for (const TCHAR* Path : CarpetAssetPaths)
	{
		ConstructorHelpers::FObjectFinder<USoundBase> SoundFinder(Path);
		if (SoundFinder.Succeeded())
		{
			CarpetFootstepSounds.Add(SoundFinder.Object);
		}
	}

	// Load Heavy Monster Footsteps
	static ConstructorHelpers::FObjectFinder<USoundBase> MonsterFootstepFinder(
		TEXT("/Game/Backrooms/Audio/Foot-steps/lordsonny-monster-footstep-162883.lordsonny-monster-footstep-162883"));
	if (MonsterFootstepFinder.Succeeded())
	{
		HeavyFootstepSounds.Add(MonsterFootstepFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> HeavyWalkingFinder(
		TEXT("/Game/Backrooms/Audio/Foot-steps/universfield-heavy-walking-footsteps-352771.universfield-heavy-walking-footsteps-352771"));
	if (HeavyWalkingFinder.Succeeded())
	{
		HeavyFootstepSounds.Add(HeavyWalkingFinder.Object);
	}

	// Load Default Fallback Footstep
	static ConstructorHelpers::FObjectFinder<USoundBase> ConcreteFootstepFinder(
		TEXT("/Game/Backrooms/Audio/Foot-steps/freeeverythingxx-walking-on-concrete-ver-2-268513.freeeverythingxx-walking-on-concrete-ver-2-268513"));
	if (ConcreteFootstepFinder.Succeeded())
	{
		DefaultFootstepSound = ConcreteFootstepFinder.Object;
	}

	// Load Breathing SFX
	static ConstructorHelpers::FObjectFinder<USoundBase> BreathingSoundFinder(
		TEXT("/Game/Backrooms/Audio/Ghost_sfx/freesound_community-sickly-breathing-83152.freesound_community-sickly-breathing-83152"));
	if (BreathingSoundFinder.Succeeded())
	{
		BreathingSound = BreathingSoundFinder.Object;
	}

	// Load Growl SFX
	static ConstructorHelpers::FObjectFinder<USoundBase> GrowlSoundFinder(
		TEXT("/Game/Backrooms/Audio/Ghost_sfx/mixkit-zombie-monster-growl-1973.mixkit-zombie-monster-growl-1973"));
	if (GrowlSoundFinder.Succeeded())
	{
		GrowlSound = GrowlSoundFinder.Object;
	}

	SetupAttenuationSettings();
}

void ABackroomsEntity::SetupAttenuationSettings()
{
	if (!FootstepAttenuation)
	{
		FootstepAttenuation = NewObject<USoundAttenuation>(this, TEXT("EntityFootstepAttenuation"));
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
	SetupAttenuationSettings();
}

void ABackroomsEntity::BeginPlay()
{
	Super::BeginPlay();

	// Initialize ambient breathing
	if (BreathingAudioComponent && BreathingSound)
	{
		BreathingAudioComponent->SetSound(BreathingSound);
		BreathingAudioComponent->SetVolumeMultiplier(0.35f);
		BreathingAudioComponent->Play();
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

	if (FootstepDistanceAccumulator >= CurrentStepDistance)
	{
		FootstepDistanceAccumulator = 0.0f;
		TriggerFootstep(CachedAIState);
	}
}

void ABackroomsEntity::TriggerFootstep(EBackroomsAIState State)
{
	const float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.0f;
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

	UPhysicalMaterial* HitPhysMat = HitResult.PhysMaterial.Get();
	const bool bIsCarpet = (HitPhysMat != nullptr) && (
		HitPhysMat == CarpetPhysicalMaterial ||
		HitPhysMat->SurfaceType == SurfaceType1 ||
		HitPhysMat->GetName().Contains(TEXT("Carpet"))
	);

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

	// Play spatialized 3D sound at floor impact point
	UGameplayStatics::PlaySoundAtLocation(
		this,
		SoundToPlay,
		HitResult.ImpactPoint,
		FRotator::ZeroRotator,
		Volume,
		Pitch,
		0.0f,
		FootstepAttenuation,
		nullptr,
		this
	);

	OnFootstepPlayed(SoundToPlay, HitResult.ImpactPoint, Volume, Pitch, bIsCarpet, State);
}
