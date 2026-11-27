// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsGameInstance.h"

UBackroomsGameInstance::UBackroomsGameInstance()
	: CurrentLevelTag(TEXT("Level0"))
{
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
