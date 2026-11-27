// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsAssetManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Engine.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "InputMappingContext.h"
#include "InputAction.h"

UBackroomsAssetManager::UBackroomsAssetManager()
{
	// Configure default soft object paths to preserve asset bindings
	CarpetPhysicalMaterial = TSoftObjectPtr<UPhysicalMaterial>(FSoftObjectPath(TEXT("/Game/Backrooms/Materials/PM_Carpet.PM_Carpet")));

	CarpetFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step1.Step1"))));
	CarpetFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step2.Step2"))));
	CarpetFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step3.Step3"))));
	CarpetFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step4.Step4"))));
	CarpetFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step5.Step5"))));

	HeavyFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/lordsonny-monster-footstep-162883.lordsonny-monster-footstep-162883"))));
	HeavyFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/universfield-heavy-walking-footsteps-352771.universfield-heavy-walking-footsteps-352771"))));

	ConcreteFootstepSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/freeeverythingxx-walking-on-concrete-ver-2-268513.freeeverythingxx-walking-on-concrete-ver-2-268513")));
	SicklyBreathingSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ghost_sfx/freesound_community-sickly-breathing-83152.freesound_community-sickly-breathing-83152")));
	ZombieGrowlSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ghost_sfx/mixkit-zombie-monster-growl-1973.mixkit-zombie-monster-growl-1973")));

	DefaultMappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_Default.IMC_Default")));
	MouseLookMappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook")));

	MoveAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Move.IA_Move")));
	LookAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Look.IA_Look")));
	MouseLookAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook")));
	JumpAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump")));
	SprintAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Sprint.IA_Sprint")));
	PauseAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Pause.IA_Pause")));

	BackroomsSoundMix = TSoftObjectPtr<USoundMix>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SM_BackroomsVolume.SM_BackroomsVolume")));
	MasterSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SoundClasses/SC_Master.SC_Master")));
	SFXSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SoundClasses/SC_SFX.SC_SFX")));
	AmbienceSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SoundClasses/SC_Ambience.SC_Ambience")));
	EntitySoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SoundClasses/SC_Entity.SC_Entity")));
}

UBackroomsAssetManager& UBackroomsAssetManager::Get()
{
	if (UBackroomsAssetManager* Singleton = Cast<UBackroomsAssetManager>(GEngine ? GEngine->AssetManager.Get() : nullptr))
	{
		return *Singleton;
	}

	return *GetMutableDefault<UBackroomsAssetManager>();
}

void UBackroomsAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	UE_LOG(LogTemp, Log, TEXT("[BackroomsAssetManager] Initializing and validating asset references..."));
	TArray<FString> MissingAssets;
	if (!ValidateAssetReferences(MissingAssets))
	{
		for (const FString& Missing : MissingAssets)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BackroomsAssetManager] Missing or unverified asset: %s"), *Missing);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[BackroomsAssetManager] All primary soft assets validated successfully in AssetRegistry."));
	}
}

bool UBackroomsAssetManager::ValidateAssetReferences(TArray<FString>& OutMissingAssets)
{
	OutMissingAssets.Empty();

	IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
	if (!AssetRegistry)
	{
		return true; // Asset registry not yet available in early phase
	}

	auto CheckSoftPath = [&](const FSoftObjectPath& SoftPath, const FString& Label)
	{
		if (SoftPath.IsNull())
		{
			return;
		}
		FAssetData AssetData = AssetRegistry->GetAssetByObjectPath(SoftPath);
		if (!AssetData.IsValid())
		{
			OutMissingAssets.Add(FString::Printf(TEXT("%s: %s"), *Label, *SoftPath.ToString()));
		}
	};

	CheckSoftPath(CarpetPhysicalMaterial.ToSoftObjectPath(), TEXT("CarpetPhysicalMaterial"));

	for (int32 i = 0; i < CarpetFootstepSounds.Num(); ++i)
	{
		CheckSoftPath(CarpetFootstepSounds[i].ToSoftObjectPath(), FString::Printf(TEXT("CarpetFootstepSounds[%d]"), i));
	}
	for (int32 i = 0; i < HeavyFootstepSounds.Num(); ++i)
	{
		CheckSoftPath(HeavyFootstepSounds[i].ToSoftObjectPath(), FString::Printf(TEXT("HeavyFootstepSounds[%d]"), i));
	}
	CheckSoftPath(ConcreteFootstepSound.ToSoftObjectPath(), TEXT("ConcreteFootstepSound"));
	CheckSoftPath(SicklyBreathingSound.ToSoftObjectPath(), TEXT("SicklyBreathingSound"));
	CheckSoftPath(ZombieGrowlSound.ToSoftObjectPath(), TEXT("ZombieGrowlSound"));

	CheckSoftPath(DefaultMappingContext.ToSoftObjectPath(), TEXT("DefaultMappingContext"));
	CheckSoftPath(MouseLookMappingContext.ToSoftObjectPath(), TEXT("MouseLookMappingContext"));
	CheckSoftPath(MoveAction.ToSoftObjectPath(), TEXT("MoveAction"));
	CheckSoftPath(LookAction.ToSoftObjectPath(), TEXT("LookAction"));
	CheckSoftPath(MouseLookAction.ToSoftObjectPath(), TEXT("MouseLookAction"));
	CheckSoftPath(JumpAction.ToSoftObjectPath(), TEXT("JumpAction"));
	CheckSoftPath(SprintAction.ToSoftObjectPath(), TEXT("SprintAction"));
	CheckSoftPath(PauseAction.ToSoftObjectPath(), TEXT("PauseAction"));

	CheckSoftPath(BackroomsSoundMix.ToSoftObjectPath(), TEXT("BackroomsSoundMix"));
	CheckSoftPath(MasterSoundClass.ToSoftObjectPath(), TEXT("MasterSoundClass"));
	CheckSoftPath(SFXSoundClass.ToSoftObjectPath(), TEXT("SFXSoundClass"));
	CheckSoftPath(AmbienceSoundClass.ToSoftObjectPath(), TEXT("AmbienceSoundClass"));
	CheckSoftPath(EntitySoundClass.ToSoftObjectPath(), TEXT("EntitySoundClass"));

	return OutMissingAssets.Num() == 0;
}

TSharedPtr<FStreamableHandle> UBackroomsAssetManager::PreloadCriticalAssetsAsync(FStreamableDelegate OnComplete)
{
	TArray<FSoftObjectPath> Targets;

	auto AddTarget = [&](const FSoftObjectPath& SoftPath)
	{
		if (!SoftPath.IsNull())
		{
			Targets.Add(SoftPath);
		}
	};

	AddTarget(CarpetPhysicalMaterial.ToSoftObjectPath());
	for (const auto& Snd : CarpetFootstepSounds) AddTarget(Snd.ToSoftObjectPath());
	for (const auto& Snd : HeavyFootstepSounds) AddTarget(Snd.ToSoftObjectPath());
	AddTarget(ConcreteFootstepSound.ToSoftObjectPath());
	AddTarget(SicklyBreathingSound.ToSoftObjectPath());
	AddTarget(ZombieGrowlSound.ToSoftObjectPath());

	AddTarget(DefaultMappingContext.ToSoftObjectPath());
	AddTarget(MouseLookMappingContext.ToSoftObjectPath());
	AddTarget(MoveAction.ToSoftObjectPath());
	AddTarget(LookAction.ToSoftObjectPath());
	AddTarget(MouseLookAction.ToSoftObjectPath());
	AddTarget(JumpAction.ToSoftObjectPath());
	AddTarget(SprintAction.ToSoftObjectPath());
	AddTarget(PauseAction.ToSoftObjectPath());

	AddTarget(BackroomsSoundMix.ToSoftObjectPath());
	AddTarget(MasterSoundClass.ToSoftObjectPath());
	AddTarget(SFXSoundClass.ToSoftObjectPath());
	AddTarget(AmbienceSoundClass.ToSoftObjectPath());
	AddTarget(EntitySoundClass.ToSoftObjectPath());

	if (Targets.Num() == 0)
	{
		OnComplete.ExecuteIfBound();
		return nullptr;
	}

	return GetStreamableManager().RequestAsyncLoad(Targets, OnComplete);
}
