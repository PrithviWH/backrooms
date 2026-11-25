// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsGameInstance.h"

UBackroomsGameInstance::UBackroomsGameInstance()
	: PersistentStamina(100.0f)
	, PersistentMaxStamina(100.0f)
	, CurrentLevelTag(TEXT("Level0"))
{
}

void UBackroomsGameInstance::SavePlayerState(float CurrentStamina, float MaxStamina)
{
	PersistentStamina = CurrentStamina;
	PersistentMaxStamina = MaxStamina;
}

void UBackroomsGameInstance::LoadPlayerState(float& OutCurrentStamina, float& OutMaxStamina) const
{
	OutCurrentStamina = PersistentStamina;
	OutMaxStamina = PersistentMaxStamina;
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
