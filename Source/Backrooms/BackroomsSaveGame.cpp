// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsSaveGame.h"
#include "BackroomsCharacter.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

const FString UBackroomsSaveGame::SlotName = TEXT("BackroomsSettings");

UBackroomsSaveGame::UBackroomsSaveGame()
	: MasterVolume(0.70f)
	, SFXVolume(0.80f)
	, AmbienceVolume(0.60f)
	, EntityVolume(0.80f)
	, OverallQualityLevel(2) // High default
	, ResolutionScale(100.0f)
	, MouseSensitivity(1.0f)
	, bInvertY(false)
{
}

UBackroomsSaveGame* UBackroomsSaveGame::LoadOrCreateSettings()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, 0))
		{
			if (UBackroomsSaveGame* Settings = Cast<UBackroomsSaveGame>(Loaded))
			{
				return Settings;
			}
		}
	}

	// Create new settings with defaults
	UBackroomsSaveGame* NewSettings = Cast<UBackroomsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UBackroomsSaveGame::StaticClass()));

	if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
	{
		NewSettings->OverallQualityLevel = UserSettings->GetOverallScalabilityLevel();
		if (NewSettings->OverallQualityLevel < 0)
		{
			NewSettings->OverallQualityLevel = 2; // High fallback
		}
		float CurrentScale = 100.0f;
		float CurrentNormalized = 1.0f;
		UserSettings->GetResolutionScaleInformationEx(CurrentNormalized, CurrentScale, CurrentScale, CurrentScale);
		NewSettings->ResolutionScale = FMath::Clamp(CurrentScale, 50.0f, 100.0f);
	}

	SaveSettings(NewSettings);
	return NewSettings;
}

bool UBackroomsSaveGame::SaveSettings(UBackroomsSaveGame* InSettings)
{
	if (!InSettings)
	{
		return false;
	}

	return UGameplayStatics::SaveGameToSlot(InSettings, SlotName, 0);
}

void UBackroomsSaveGame::ApplyAllSettings(UObject* WorldContextObject, UBackroomsSaveGame* InSettings)
{
	if (!InSettings)
	{
		return;
	}

	ApplyAudioSettings(WorldContextObject, InSettings->MasterVolume, InSettings->SFXVolume, InSettings->AmbienceVolume, InSettings->EntityVolume);
	ApplyGraphicsSettings(InSettings->OverallQualityLevel, InSettings->ResolutionScale);
	ApplyControlsSettings(WorldContextObject, InSettings->MouseSensitivity, InSettings->bInvertY);
}

void UBackroomsSaveGame::ApplyAudioSettings(UObject* WorldContextObject, float InMaster, float InSFX, float InAmbience, float InEntity)
{
	if (!WorldContextObject)
	{
		return;
	}

	static USoundMix* SoundMix = nullptr;
	if (!SoundMix)
	{
		SoundMix = LoadObject<USoundMix>(nullptr, TEXT("/Game/Backrooms/Audio/SM_BackroomsVolume.SM_BackroomsVolume"));
	}

	if (SoundMix)
	{
		UGameplayStatics::PushSoundMixModifier(WorldContextObject, SoundMix);

		// SC_Master
		if (USoundClass* SC_Master = LoadObject<USoundClass>(nullptr, TEXT("/Game/Backrooms/Audio/SoundClasses/SC_Master.SC_Master")))
		{
			UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, SoundMix, SC_Master, FMath::Clamp(InMaster, 0.0f, 1.0f), 1.0f, 0.0f, true);
		}

		// SC_SFX
		if (USoundClass* SC_SFX = LoadObject<USoundClass>(nullptr, TEXT("/Game/Backrooms/Audio/SoundClasses/SC_SFX.SC_SFX")))
		{
			UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, SoundMix, SC_SFX, FMath::Clamp(InSFX, 0.0f, 1.0f), 1.0f, 0.0f, true);
		}

		// SC_Ambience
		if (USoundClass* SC_Ambience = LoadObject<USoundClass>(nullptr, TEXT("/Game/Backrooms/Audio/SoundClasses/SC_Ambience.SC_Ambience")))
		{
			UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, SoundMix, SC_Ambience, FMath::Clamp(InAmbience, 0.0f, 1.0f), 1.0f, 0.0f, true);
		}

		// SC_Entity
		if (USoundClass* SC_Entity = LoadObject<USoundClass>(nullptr, TEXT("/Game/Backrooms/Audio/SoundClasses/SC_Entity.SC_Entity")))
		{
			UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, SoundMix, SC_Entity, FMath::Clamp(InEntity, 0.0f, 1.0f), 1.0f, 0.0f, true);
		}
	}
}

void UBackroomsSaveGame::ApplyGraphicsSettings(int32 QualityLevel, float InResolutionScale)
{
	if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
	{
		if (QualityLevel >= 0 && QualityLevel <= 3)
		{
			UserSettings->SetOverallScalabilityLevel(QualityLevel);
		}

		UserSettings->SetResolutionScaleValueEx(FMath::Clamp(InResolutionScale, 50.0f, 100.0f));
		UserSettings->ApplySettings(false);
	}
}

void UBackroomsSaveGame::ApplyControlsSettings(UObject* WorldContextObject, float InSensitivity, bool bInInvertY)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (ABackroomsCharacter* BackroomsChar = Cast<ABackroomsCharacter>(PC->GetPawn()))
			{
				BackroomsChar->SetMouseSensitivity(InSensitivity);
				BackroomsChar->SetInvertY(bInInvertY);
			}
		}
	}
}
