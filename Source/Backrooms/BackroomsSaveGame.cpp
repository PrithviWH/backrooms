// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsSaveGame.h"
#include "BackroomsAssetManager.h"
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
	, MouseSensitivity(0.20f)
	, bInvertY(false)
{
}

UBackroomsSaveGame* UBackroomsSaveGame::LoadOrCreateSettings()
{
	static FCriticalSection LoadSaveLock;
	FScopeLock Lock(&LoadSaveLock);

	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, 0))
		{
			if (UBackroomsSaveGame* Settings = Cast<UBackroomsSaveGame>(Loaded))
			{
				// Migrate old high sensitivity default (1.0f) to comfortable new default (0.20f)
				if (FMath::IsNearlyEqual(Settings->MouseSensitivity, 1.0f, 0.05f))
				{
					Settings->MouseSensitivity = 0.20f;
				}
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

	UGameplayStatics::AsyncSaveGameToSlot(NewSettings, SlotName, 0);
	return NewSettings;
}

bool UBackroomsSaveGame::SaveSettings(UBackroomsSaveGame* InSettings)
{
	if (!InSettings)
	{
		return false;
	}

	static FCriticalSection SaveGameLock;
	FScopeLock Lock(&SaveGameLock);

	UGameplayStatics::AsyncSaveGameToSlot(InSettings, SlotName, 0);
	return true;
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

TObjectPtr<USoundMix> UBackroomsSaveGame::CachedSoundMix = nullptr;
TObjectPtr<USoundClass> UBackroomsSaveGame::CachedSC_Master = nullptr;
TObjectPtr<USoundClass> UBackroomsSaveGame::CachedSC_SFX = nullptr;
TObjectPtr<USoundClass> UBackroomsSaveGame::CachedSC_Ambience = nullptr;
TObjectPtr<USoundClass> UBackroomsSaveGame::CachedSC_Entity = nullptr;

void UBackroomsSaveGame::ApplyAudioSettings(UObject* WorldContextObject, float InMaster, float InSFX, float InAmbience, float InEntity)
{
	if (!WorldContextObject)
	{
		return;
	}

	if (!CachedSoundMix)
	{
		CachedSoundMix = UBackroomsAssetManager::Get().BackroomsSoundMix.LoadSynchronous();
	}

	if (CachedSoundMix)
	{
		UGameplayStatics::PushSoundMixModifier(WorldContextObject, CachedSoundMix);

		// SC_Master
		if (!CachedSC_Master)
		{
			CachedSC_Master = UBackroomsAssetManager::Get().MasterSoundClass.LoadSynchronous();
		}
		if (CachedSC_Master)
		{
			UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, CachedSoundMix, CachedSC_Master, FMath::Clamp(InMaster, 0.0f, 1.0f), 1.0f, 0.0f, true);
		}

		// SC_SFX
		if (!CachedSC_SFX)
		{
			CachedSC_SFX = UBackroomsAssetManager::Get().SFXSoundClass.LoadSynchronous();
		}
		if (CachedSC_SFX)
		{
			UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, CachedSoundMix, CachedSC_SFX, FMath::Clamp(InSFX, 0.0f, 1.0f), 1.0f, 0.0f, true);
		}

		// SC_Ambience
		if (!CachedSC_Ambience)
		{
			CachedSC_Ambience = UBackroomsAssetManager::Get().AmbienceSoundClass.LoadSynchronous();
		}
		if (CachedSC_Ambience)
		{
			UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, CachedSoundMix, CachedSC_Ambience, FMath::Clamp(InAmbience, 0.0f, 1.0f), 1.0f, 0.0f, true);
		}

		// SC_Entity
		if (!CachedSC_Entity)
		{
			CachedSC_Entity = UBackroomsAssetManager::Get().EntitySoundClass.LoadSynchronous();
		}
		if (CachedSC_Entity)
		{
			UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, CachedSoundMix, CachedSC_Entity, FMath::Clamp(InEntity, 0.0f, 1.0f), 1.0f, 0.0f, true);
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
