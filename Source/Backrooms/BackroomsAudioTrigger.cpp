// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsAudioTrigger.h"
#include "BackroomsAssetManager.h"
#include "BackroomsCharacter.h"
#include "Components/SphereComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundClass.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ABackroomsAudioTrigger::ABackroomsAudioTrigger()
	: SoundCategory(EBackroomsTriggerSoundCategory::MixedRandom)
	, MinCooldown(35.0f)
	, MaxCooldown(65.0f)
	, TriggerChance(0.85f)
	, VolumeMultiplier(0.38f)
	, SpatialAudioOffset(FVector(0.0f, 0.0f, 80.0f))
	, FalloffDistance(2600.0f)
	, bOneShotOnly(false)
	, bHasFired(false)
	, CooldownTimer(0.0f)
	, SpatialAttenuation(nullptr)
	, AmbienceSoundClass(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerVolume"));
	RootComponent = TriggerVolume;
	TriggerVolume->SetSphereRadius(350.0f);
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ABackroomsAudioTrigger::BeginPlay()
{
	Super::BeginPlay();

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ABackroomsAudioTrigger::OnTriggerOverlap);

	// Setup 3D spatial attenuation with realistic liminal acoustic falloff
	SpatialAttenuation = NewObject<USoundAttenuation>(this, TEXT("TriggerSpatialAttenuation"));
	if (SpatialAttenuation)
	{
		FSoundAttenuationSettings& Settings = SpatialAttenuation->Attenuation;
		Settings.bAttenuate = true;
		Settings.bSpatialize = true;
		Settings.SpatializationAlgorithm = ESoundSpatializationAlgorithm::SPATIALIZATION_Default;
		Settings.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
		Settings.AttenuationShape = EAttenuationShape::Sphere;
		Settings.FalloffDistance = FalloffDistance;
		Settings.bAttenuateWithLPF = true;
		Settings.LPFRadiusMin = 400.0f;
		Settings.LPFRadiusMax = FalloffDistance;
	}

	UBackroomsAssetManager& AssetManager = UBackroomsAssetManager::Get();
	AmbienceSoundClass = AssetManager.AmbienceSoundClass.LoadSynchronous();
}

void ABackroomsAudioTrigger::OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || !OtherActor->IsA(ABackroomsCharacter::StaticClass()))
	{
		return;
	}

	if (bHasFired && bOneShotOnly)
	{
		return;
	}

	if (GetWorld()->GetTimerManager().IsTimerActive(CooldownTimerHandle))
	{
		return; // Currently on cooldown
	}

	// Probability check for organic unpredictability
	if (FMath::FRand() > TriggerChance)
	{
		return;
	}

	TriggerSound(OtherActor);
}

void ABackroomsAudioTrigger::TriggerSound(AActor* InstigatorActor)
{
	USoundBase* SoundToPlay = SelectRandomSound();
	if (!SoundToPlay)
	{
		return;
	}

	const FVector PlayLocation = GetActorLocation() + SpatialAudioOffset;
	const float Volume = FMath::Clamp(VolumeMultiplier, 0.0f, 0.5f); // Strictly <= 0.5 per AGENTS.md
	const float Pitch = FMath::RandRange(0.93f, 1.05f);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		SoundToPlay,
		PlayLocation,
		FRotator::ZeroRotator,
		Volume,
		Pitch,
		0.0f,
		SpatialAttenuation,
		nullptr,
		this
	);

	bHasFired = true;

	if (!bOneShotOnly)
	{
		const float Cooldown = FMath::RandRange(MinCooldown, MaxCooldown);
		GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, &ABackroomsAudioTrigger::ResetCooldown, Cooldown, false);
	}

	UE_LOG(LogTemp, Log, TEXT("[AudioTrigger] Fired 3D sound '%s' at %s (Vol=%.2f, Pitch=%.2f)"),
		*SoundToPlay->GetName(), *PlayLocation.ToCompactString(), Volume, Pitch);
}

void ABackroomsAudioTrigger::ResetCooldown()
{
	bHasFired = false;
}

USoundBase* ABackroomsAudioTrigger::SelectRandomSound()
{
	UBackroomsAssetManager& AM = UBackroomsAssetManager::Get();

	if (SoundCategory == EBackroomsTriggerSoundCategory::SpecificSound)
	{
		return SpecificSoundAsset.LoadSynchronous();
	}

	TArray<USoundBase*> CandidateSounds;

	auto AddValid = [&](TSoftObjectPtr<USoundBase>& SoftPtr)
	{
		if (USoundBase* Snd = SoftPtr.LoadSynchronous())
		{
			CandidateSounds.Add(Snd);
		}
	};

	if (SoundCategory == EBackroomsTriggerSoundCategory::CreepyVoices || SoundCategory == EBackroomsTriggerSoundCategory::MixedRandom)
	{
		AddValid(AM.VoiceWhisperOverHere);
		AddValid(AM.VoiceWhisperDontLookBack);
		AddValid(AM.VoiceWhisperNoEscape);
		AddValid(AM.VoiceWhisperCounting);
		AddValid(AM.VoiceWhisperWhyRunning);
		AddValid(AM.CreepyWhisperSenseSound);
		AddValid(AM.CreepyPossessedLaughSound);
	}

	if (SoundCategory == EBackroomsTriggerSoundCategory::SubtleSFX || SoundCategory == EBackroomsTriggerSoundCategory::MixedRandom)
	{
		AddValid(AM.SFXSubtlePipeDrip);
		AddValid(AM.SFXSubtleWallGroan);
		AddValid(AM.SFXSubtleVentDraft);
		AddValid(AM.SFXSubtleLightArc);
		AddValid(AM.SFXSubtleCeilingScuttle);
		AddValid(AM.CreepyDoorKnockSound);
		AddValid(AM.CreepyDistantThudSound);
	}

	if (CandidateSounds.Num() == 0)
	{
		return nullptr;
	}

	const int32 Index = FMath::RandRange(0, CandidateSounds.Num() - 1);
	return CandidateSounds[Index];
}
