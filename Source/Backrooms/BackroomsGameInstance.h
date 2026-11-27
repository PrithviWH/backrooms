// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BackroomsGameInstance.generated.h"

/**
 * Custom GameInstance that persists gameplay state (inventory, level progression)
 * across level transitions (e.g. from L_Backrooms_Main to backroomsLevel2).
 * Strictly contains session-lifetime data and persistence helpers.
 */
UCLASS()
class BACKROOMS_API UBackroomsGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UBackroomsGameInstance();

	/** Persistent inventory item identifiers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FString> PersistentInventory;

	/** Current level or checkpoint tag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	FName CurrentLevelTag;

	/** Saves character state to the GameInstance (stamina parameters preserved for backwards compatibility) */
	UFUNCTION(BlueprintCallable, Category = "Backrooms|Persistence")
	void SavePlayerState(float CurrentStamina = 100.0f, float MaxStamina = 100.0f);

	/** Loads persisted character state */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Backrooms|Persistence")
	void LoadPlayerState(float& OutCurrentStamina, float& OutMaxStamina) const;

	/** Adds an item to the persistent inventory */
	UFUNCTION(BlueprintCallable, Category = "Backrooms|Persistence")
	void AddInventoryItem(const FString& ItemId);

	/** Checks if an item is present in persistent inventory */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Backrooms|Persistence")
	bool HasInventoryItem(const FString& ItemId) const;
};
