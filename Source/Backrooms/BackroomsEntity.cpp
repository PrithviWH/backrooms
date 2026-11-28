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
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "BackroomsCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "EngineUtils.h"

ABackroomsEntity::ABackroomsEntity()
	: WanderStepDistance(175.0f)
	, SearchStepDistance(145.0f)
	, ChaseStepDistance(125.0f)
	, WanderFootstepVolume(0.55f)
	, SearchFootstepVolume(0.72f)
	, ChaseFootstepVolume(1.05f)
	, CachedAIState(EBackroomsAIState::IdleWander)
	, LightFlickerRadius(600.0f)
	, LightBlackoutRadius(180.0f)
	, bEnableLightDistortion(true)
	, AttackRange(140.0f)
	, AttackCooldown(3.5f)
	, bIsAttacking(false)
	, LitSilhouetteColor(FLinearColor(0.018f, 0.015f, 0.012f, 1.0f))
	, UnlitSilhouetteColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f))
	, LitRoughness(0.92f)
	, UnlitRoughness(1.0f)
	, LitSpecular(0.08f)
	, UnlitSpecular(0.0f)
	, bIsIlluminatedByLight(false)
	, CurrentSilhouetteColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f))
	, CurrentRoughness(1.0f)
	, CurrentSpecular(0.0f)
	, FootstepDistanceAccumulator(0.0f)
	, LastCarpetSoundIndex(-1)
	, LastHeavySoundIndex(-1)
	, NextFootstepPoolIndex(0)
	, LastFootstepTime(0.0f)
	, AttackCooldownTimer(0.0f)
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

	// Configure capsule size for humanoid monster
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->CanCharacterStepUpOn = ECB_No;

	// Position and orient skeletal mesh: -96cm down to floor, -90 deg yaw to face actor forward
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		MeshComp->bCastDynamicShadow = true;
	}

	// Disable controller yaw snapping so the character turns naturally along its path
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure movement component for physical walking/running (prevents gliding/flying)
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DefaultLandMovementMode = MOVE_Walking;
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->GravityScale = 1.0f;
		MoveComp->MaxAcceleration = 2048.0f;
		MoveComp->BrakingDecelerationWalking = 2048.0f;
		MoveComp->GroundFriction = 8.0f;
		MoveComp->bRequestedMoveUseAcceleration = true;
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 480.0f, 0.0f);
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->MaxWalkSpeed = 180.0f;
	}
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
	if (!AttackScreamSound)
	{
		AttackScreamSound = AssetManager.JumpscareScreamSound.LoadSynchronous();
	}
	if (!AttackHitSound)
	{
		AttackHitSound = AssetManager.JumpscareHitSound.LoadSynchronous();
	}
	if (!AttackAnimation)
	{
		AttackAnimation = AssetManager.MonsterAttackAnimation.LoadSynchronous();
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

	// Enforce floor alignment and forward orientation
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	// Enforce walking movement mode and orientation to movement
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->bOrientRotationToMovement = true;
	}

	// Shroud monster in pitch-black shadow void to obscure features and maximize panic
	ApplyShadowVoidMaterial();

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

	// Cache all nearby level lights to enable dynamic flickering and blackout
	if (bEnableLightDistortion)
	{
		InitializeTrackedLights();
	}
}

void ABackroomsEntity::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Restore all lights to their original baseline intensity upon entity destruction or level transition
	for (FBackroomsTrackedLight& Light : TrackedLights)
	{
		if (UPointLightComponent* PLC = Light.LightComponent.Get())
		{
			PLC->SetIntensity(Light.BaseIntensity);
		}
	}
	TrackedLights.Empty();

	Super::EndPlay(EndPlayReason);
}

void ABackroomsEntity::InitializeTrackedLights()
{
	TrackedLights.Empty();

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APointLight> It(World); It; ++It)
		{
			APointLight* LightActor = *It;
			if (LightActor && LightActor->PointLightComponent)
			{
				UPointLightComponent* PLC = LightActor->PointLightComponent;
				FBackroomsTrackedLight Tracked;
				Tracked.LightComponent = PLC;
				Tracked.BaseIntensity = PLC->Intensity;
				Tracked.Location = LightActor->GetActorLocation();
				TrackedLights.Add(Tracked);
			}
		}
	}
}

void ABackroomsEntity::UpdateLightDistortion(float DeltaTime)
{
	if (TrackedLights.Num() == 0)
	{
		return;
	}

	const FVector EntityLoc = GetActorLocation();
	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float FlickerRadiusSq = FMath::Square(LightFlickerRadius);

	for (FBackroomsTrackedLight& Light : TrackedLights)
	{
		UPointLightComponent* PLC = Light.LightComponent.Get();
		if (!PLC)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(EntityLoc, Light.Location);

		if (DistSq < FlickerRadiusSq)
		{
			const float Dist = FMath::Sqrt(DistSq);

			if (Dist < LightBlackoutRadius)
			{
				// Blackout zone (<5m): The entity's anomalous presence extinguishes the bulb, with rare faint sparks
				const float Spark = (FMath::FRand() < 0.05f) ? FMath::FRandRange(0.1f, 0.3f) : 0.0f;
				PLC->SetIntensity(Light.BaseIntensity * Spark);
			}
			else
			{
				// Flicker zone (5m to 14m): Erratic rapid Perlin oscillation + intermittent ballast dropouts
				const float NormalizedDist = (Dist - LightBlackoutRadius) / (LightFlickerRadius - LightBlackoutRadius); // 0 at blackout, 1 at boundary
				const float SpatialSeed = (Light.Location.X * 0.013f + Light.Location.Y * 0.027f);
				const float Noise = FMath::PerlinNoise1D(Time * 28.0f + SpatialSeed) * 0.5f + 0.5f;

				// Random micro-dropout simulating arcing fluorescent starter
				float Dropout = 1.0f;
				if (FMath::FRand() < 0.15f * (1.0f - NormalizedDist))
				{
					Dropout = FMath::FRandRange(0.05f, 0.25f);
				}

				const float TargetFactor = FMath::Lerp(Noise * Dropout, 1.0f, NormalizedDist * NormalizedDist);
				PLC->SetIntensity(Light.BaseIntensity * TargetFactor);
			}
		}
		else
		{
			// Outside distortion envelope: smoothly restore back to original baseline intensity
			if (!FMath::IsNearlyEqual(PLC->Intensity, Light.BaseIntensity, 1.0f))
			{
				PLC->SetIntensity(FMath::FInterpTo(PLC->Intensity, Light.BaseIntensity, DeltaTime, 8.0f));
			}
		}
	}
}

void ABackroomsEntity::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update attack cooldown
	AttackCooldownTimer = FMath::Max(0.0f, AttackCooldownTimer - DeltaTime);

	// Update cached AI state from controller
	if (ABackroomsAIController* AICon = Cast<ABackroomsAIController>(GetController()))
	{
		CachedAIState = AICon->CurrentState;
	}

	// Modulate ambient breathing volume: mute when far or behind, only audible up close
	if (BreathingAudioComponent && BreathingAudioComponent->IsPlaying())
	{
		float TargetVolume = 0.0f;
		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (APawn* PlayerPawn = PC->GetPawn())
				{
					const float DistToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
					if (DistToPlayer < 450.0f)
					{
						TargetVolume = (CachedAIState == EBackroomsAIState::Chase) ? 0.70f : 0.35f;
					}
				}
			}
		}
		BreathingAudioComponent->SetVolumeMultiplier(FMath::FInterpTo(BreathingAudioComponent->VolumeMultiplier, TargetVolume, DeltaTime, 4.0f));
	}

	UpdateFootsteps(DeltaTime);

	// Update lighting-dependent silhouette shading (pitch black in unlit areas/lights off, dark humanoid silhouette under light)
	UpdateLightingSilhouette(DeltaTime);

	// Modulate and distort nearby fluorescent lights
	if (bEnableLightDistortion)
	{
		UpdateLightDistortion(DeltaTime);
	}
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

void ABackroomsEntity::ApplyShadowVoidMaterial()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	DynamicMaterials.Empty();

	// Load dedicated pure shadow void master material (M_EntityShadowVoid)
	UMaterialInterface* BaseVoidMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Backrooms/Materials/M_EntityShadowVoid.M_EntityShadowVoid"));

	const int32 NumMats = MeshComp->GetNumMaterials();
	for (int32 SlotIdx = 0; SlotIdx < NumMats; ++SlotIdx)
	{
		if (BaseVoidMat)
		{
			MeshComp->SetMaterial(SlotIdx, BaseVoidMat);
		}

		UMaterialInstanceDynamic* DynMat = MeshComp->CreateAndSetMaterialInstanceDynamic(SlotIdx);
		if (DynMat)
		{
			// Initialize with pure black unlit silhouette parameters
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), CurrentSilhouetteColor);
			DynMat->SetScalarParameterValue(FName(TEXT("Roughness")), CurrentRoughness);
			DynMat->SetScalarParameterValue(FName(TEXT("Specular")), CurrentSpecular);
			DynMat->SetScalarParameterValue(FName(TEXT("Metallic")), 0.0f);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive")), FLinearColor::Black);

			// Fallback parameters in case base material retained parameter hooks
			DynMat->SetVectorParameterValue(FName(TEXT("Paint Tint")), CurrentSilhouetteColor);
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), CurrentSilhouetteColor);
			DynMat->SetVectorParameterValue(FName(TEXT("Tint")), CurrentSilhouetteColor);
			DynMat->SetVectorParameterValue(FName(TEXT("LogoTint")), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			DynMat->SetScalarParameterValue(FName(TEXT("LogoScale")), 0.0f);
			DynMat->SetScalarParameterValue(FName(TEXT("MetalPaintMetallic")), 0.0f);
			DynMat->SetScalarParameterValue(FName(TEXT("MetalPaintRoughness")), CurrentRoughness);
			DynMat->SetScalarParameterValue(FName(TEXT("EmissivePower")), 0.0f);

			DynamicMaterials.Add(DynMat);
		}
	}
}

void ABackroomsEntity::UpdateLightingSilhouette(float DeltaTime)
{
	if (DynamicMaterials.Num() == 0)
	{
		return;
	}

	const FVector EntityLoc = GetActorLocation();
	const FVector ChestLoc = EntityLoc + FVector(0.0f, 0.0f, 50.0f);

	bool bFoundActiveLight = false;

	for (const FBackroomsTrackedLight& Light : TrackedLights)
	{
		UPointLightComponent* PLC = Light.LightComponent.Get();
		if (!PLC)
		{
			continue;
		}

		// Light must be currently turned on and active (intensity > 15.0) to illuminate entity
		if (PLC->Intensity <= 15.0f)
		{
			continue;
		}

		const float AttenRadius = PLC->AttenuationRadius;
		const float DistSq = FVector::DistSquared(EntityLoc, Light.Location);

		if (DistSq <= FMath::Square(AttenRadius))
		{
			// Line of sight check between light source and entity chest to ensure not occluded by walls
			if (UWorld* World = GetWorld())
			{
				FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(LightOcclusionTrace), false, this);
				FHitResult HitResult;
				const bool bBlocked = World->LineTraceSingleByChannel(HitResult, Light.Location, ChestLoc, ECC_Visibility, TraceParams);

				if (!bBlocked)
				{
					bFoundActiveLight = true;
					break; // Found an active, unobstructed light illuminating entity
				}
			}
		}
	}

	bIsIlluminatedByLight = bFoundActiveLight;

	// Target shading parameters based on active illumination
	// Under active ceiling light: dark obsidian silhouette that catches light contours
	// When light directly above is off or in unlit areas: 100% PITCH BLACK, zero specular, pure featureless silhouette
	const FLinearColor TargetColor = bIsIlluminatedByLight ? LitSilhouetteColor : UnlitSilhouetteColor;
	const float TargetRoughness = bIsIlluminatedByLight ? LitRoughness : UnlitRoughness;
	const float TargetSpecular = bIsIlluminatedByLight ? LitSpecular : UnlitSpecular;

	// Rapid smooth interpolation so flickers and blackouts transition seamlessly
	CurrentSilhouetteColor = FMath::CInterpTo(CurrentSilhouetteColor, TargetColor, DeltaTime, 10.0f);
	CurrentRoughness = FMath::FInterpTo(CurrentRoughness, TargetRoughness, DeltaTime, 10.0f);
	CurrentSpecular = FMath::FInterpTo(CurrentSpecular, TargetSpecular, DeltaTime, 10.0f);

	for (UMaterialInstanceDynamic* DynMat : DynamicMaterials)
	{
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), CurrentSilhouetteColor);
			DynMat->SetScalarParameterValue(FName(TEXT("Roughness")), CurrentRoughness);
			DynMat->SetScalarParameterValue(FName(TEXT("Specular")), CurrentSpecular);
			DynMat->SetScalarParameterValue(FName(TEXT("Metallic")), 0.0f);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive")), FLinearColor::Black);

			// Fallback parameters in case base material retained parameter hooks
			DynMat->SetVectorParameterValue(FName(TEXT("Paint Tint")), CurrentSilhouetteColor);
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), CurrentSilhouetteColor);
			DynMat->SetVectorParameterValue(FName(TEXT("Tint")), CurrentSilhouetteColor);
			DynMat->SetScalarParameterValue(FName(TEXT("MetalPaintRoughness")), CurrentRoughness);
		}
	}
}

void ABackroomsEntity::SetStalkerVisibility(bool bVisible)
{
	SetActorHiddenInGame(!bVisible);
	SetActorEnableCollision(bVisible);

	if (BreathingAudioComponent)
	{
		if (!bVisible)
		{
			BreathingAudioComponent->SetVolumeMultiplier(0.0f);
			BreathingAudioComponent->Stop();
		}
		else
		{
			BreathingAudioComponent->SetVolumeMultiplier(0.35f);
			if (BreathingSound)
			{
				BreathingAudioComponent->SetSound(BreathingSound);
			}
			BreathingAudioComponent->Play();
		}
	}
}

bool ABackroomsEntity::ExecuteAttack(ABackroomsCharacter* TargetPlayerChar)
{
	if (!TargetPlayerChar || bIsAttacking || AttackCooldownTimer > 0.0f || IsActorBeingDestroyed())
	{
		return false;
	}

	bIsAttacking = true;
	AttackCooldownTimer = AttackCooldown;

	// Play lunging strike animation via dynamic slot montage
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
		{
			if (AttackAnimation)
			{
				AnimInst->PlaySlotAnimationAsDynamicMontage(AttackAnimation, FName(TEXT("DefaultSlot")), 0.15f, 0.2f, 1.25f);
			}
		}
	}

	// Play horrifying jumpscare scream & impact sound at monster location
	if (AttackScreamSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackScreamSound, GetActorLocation(), 1.35f, FMath::RandRange(0.92f, 1.05f));
	}
	if (AttackHitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackHitSound, GetActorLocation(), 1.5f, 1.0f);
	}

	// Dispatch caught jumpscare sequence to player character
	TargetPlayerChar->TriggerJumpscareCaught(this);

	// Reset attacking flag after animation completes
	if (UWorld* World = GetWorld())
	{
		FTimerHandle AttackTimerHandle;
		World->GetTimerManager().SetTimer(AttackTimerHandle, [this]()
		{
			bIsAttacking = false;
		}, 1.5f, false);
	}

	return true;
}
