// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BackroomsGameInstance.generated.h"

class APlayerController;

/**
 * Custom GameInstance that persists gameplay state (stamina, inventory, level progression)
 * across level transitions (e.g. from L_Backrooms_Main to backroomsLevel2) and provides
 * smooth fade-to-black camera transition helpers.
 */
UCLASS()
class BACKROOMS_API UBackroomsGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UBackroomsGameInstance();

	/** Persistent player stamina value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player State")
	float PersistentStamina;

	/** Persistent player max stamina */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player State")
	float PersistentMaxStamina;

	/** Persistent inventory item identifiers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FString> PersistentInventory;

	/** Current level or checkpoint tag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	FName CurrentLevelTag;

	/** Saves character stamina to the GameInstance */
	UFUNCTION(BlueprintCallable, Category = "Backrooms|Persistence")
	void SavePlayerState(float CurrentStamina, float MaxStamina);

	/** Loads persisted character stamina */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Backrooms|Persistence")
	void LoadPlayerState(float& OutCurrentStamina, float& OutMaxStamina) const;

	/** Adds an item to the persistent inventory */
	UFUNCTION(BlueprintCallable, Category = "Backrooms|Persistence")
	void AddInventoryItem(const FString& ItemId);

	/** Checks if an item is present in persistent inventory */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Backrooms|Persistence")
	bool HasInventoryItem(const FString& ItemId) const;

	/** Performs camera fade to black on the player camera manager (holds at black) */
	UFUNCTION(BlueprintCallable, Category = "Backrooms|Camera")
	static void FadeCameraToBlack(APlayerController* PlayerController, float Duration = 1.0f);

	/** Performs camera fade from black on the player camera manager */
	UFUNCTION(BlueprintCallable, Category = "Backrooms|Camera")
	static void FadeCameraFromBlack(APlayerController* PlayerController, float Duration = 1.0f);

	/** Initiates a level transition with camera fade-to-black before loading */
	UFUNCTION(BlueprintCallable, Category = "Backrooms|Transition", meta = (WorldContext = "WorldContextObject"))
	static void TransitionToLevelWithFade(const UObject* WorldContextObject, FName LevelName, float FadeDuration = 1.0f);
};
