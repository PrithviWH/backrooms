// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

void UBackroomsGameInstance::FadeCameraToBlack(APlayerController* PlayerController, float Duration)
{
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		// From 0.0 to 1.0, black color, hold when finished
		PlayerController->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, Duration, FLinearColor::Black, false, true);
	}
}

void UBackroomsGameInstance::FadeCameraFromBlack(APlayerController* PlayerController, float Duration)
{
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		// From 1.0 to 0.0, black color, do not hold
		PlayerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, Duration, FLinearColor::Black, false, false);
	}
}

void UBackroomsGameInstance::TransitionToLevelWithFade(const UObject* WorldContextObject, FName LevelName, float FadeDuration)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	FadeCameraToBlack(PC, FadeDuration);

	FTimerHandle TransitionTimerHandle;
	World->GetTimerManager().SetTimer(TransitionTimerHandle, [WorldContextObject, LevelName]()
	{
		UGameplayStatics::OpenLevel(WorldContextObject, LevelName);
	}, FadeDuration, false);
}
