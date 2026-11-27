// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "BackroomsAssetManager.generated.h"

class UPhysicalMaterial;
class USoundBase;
class USoundMix;
class USoundClass;
class UInputMappingContext;
class UInputAction;

/**
 * Central asset manager for Backrooms 5.8.
 * Provides soft object paths and async loading for all primary project assets,
 * replacing hardcoded asset strings and enabling runtime validation via AssetRegistry.
 */
UCLASS()
class BACKROOMS_API UBackroomsAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	UBackroomsAssetManager();

	/** Static accessor to project asset manager singleton or CDO fallback */
	static UBackroomsAssetManager& Get();

	virtual void StartInitialLoading() override;

	// --- Physical Materials ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Physics")
	TSoftObjectPtr<UPhysicalMaterial> CarpetPhysicalMaterial;

	// --- Footstep & AI Sounds ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TArray<TSoftObjectPtr<USoundBase>> CarpetFootstepSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TArray<TSoftObjectPtr<USoundBase>> HeavyFootstepSounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> ConcreteFootstepSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> SicklyBreathingSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Audio")
	TSoftObjectPtr<USoundBase> ZombieGrowlSound;

	// --- Input Assets ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputMappingContext> MouseLookMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|Input")
	TSoftObjectPtr<UInputAction> PauseAction;

	// --- Audio Mix & Sound Classes ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|AudioMix")
	TSoftObjectPtr<USoundMix> BackroomsSoundMix;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|AudioMix")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|AudioMix")
	TSoftObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|AudioMix")
	TSoftObjectPtr<USoundClass> AmbienceSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assets|AudioMix")
	TSoftObjectPtr<USoundClass> EntitySoundClass;

	/** Validates that configured soft paths point to existing assets in the AssetRegistry */
	UFUNCTION(BlueprintCallable, Category = "Assets")
	bool ValidateAssetReferences(TArray<FString>& OutMissingAssets);

	/** Preloads all critical soft assets asynchronously */
	TSharedPtr<FStreamableHandle> PreloadCriticalAssetsAsync(FStreamableDelegate OnComplete = FStreamableDelegate());
};
