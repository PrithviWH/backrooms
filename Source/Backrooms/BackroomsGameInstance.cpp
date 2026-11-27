// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsGameInstance.h"

UBackroomsGameInstance::UBackroomsGameInstance()
	: CurrentLevelTag(TEXT("Level0"))
{
}

void UBackroomsGameInstance::SavePlayerState(float CurrentStamina, float MaxStamina)
{
	// Stamina is intentionally ungated/unconstrained; stub preserved for blueprint compatibility
}

void UBackroomsGameInstance::LoadPlayerState(float& OutCurrentStamina, float& OutMaxStamina) const
{
	OutCurrentStamina = 100.0f;
	OutMaxStamina = 100.0f;
}

void UBackroomsGameInstance::AddInventoryItem(const FString& ItemId)
{
	if (!ItemId.IsEmpty() && !PersistentInventory.Contains(ItemId))
	{
		PersistentInventory.Add(ItemId);
	}
}

bool UBackroomsGameInstance::HasInventoryItem(const FString& ItemId) const
{
	return PersistentInventory.Contains(ItemId);
}
