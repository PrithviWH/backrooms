// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BackroomsSaveGame.generated.h"

class USoundMix;
class USoundClass;

/**
 * Persistent settings save game for Backrooms 5.8.
 * Stores audio volumes, graphics scalability presets, and control preferences.
 */
UCLASS()
class BACKROOMS_API UBackroomsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UBackroomsSaveGame();

	// --- Audio Settings (0.0 to 1.0) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Audio")
	float MasterVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Audio")
	float SFXVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Audio")
	float AmbienceVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Audio")
	float EntityVolume;

	// --- Graphics Settings ---
	// 0: Low, 1: Medium, 2: High, 3: Epic
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Graphics")
	int32 OverallQualityLevel;

	// 50.0 to 100.0 percent
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Graphics")
	float ResolutionScale;

	// --- Controls Settings ---
	// 0.1 to 3.0 (default 1.0)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Controls")
	float MouseSensitivity;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Controls")
	bool bInvertY;

	// --- Helper Functions ---
	static const FString SlotName;

	UFUNCTION(BlueprintCallable, Category = "Settings")
	static UBackroomsSaveGame* LoadOrCreateSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	static bool SaveSettings(UBackroomsSaveGame* InSettings);

	UFUNCTION(BlueprintCallable, Category = "Settings", meta = (WorldContext = "WorldContextObject"))
	static void ApplyAllSettings(UObject* WorldContextObject, UBackroomsSaveGame* InSettings);

	UFUNCTION(BlueprintCallable, Category = "Settings", meta = (WorldContext = "WorldContextObject"))
	static void ApplyAudioSettings(UObject* WorldContextObject, float InMaster, float InSFX, float InAmbience, float InEntity);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	static void ApplyGraphicsSettings(int32 QualityLevel, float InResolutionScale);

	UFUNCTION(BlueprintCallable, Category = "Settings", meta = (WorldContext = "WorldContextObject"))
	static void ApplyControlsSettings(UObject* WorldContextObject, float InSensitivity, bool bInInvertY);
};
