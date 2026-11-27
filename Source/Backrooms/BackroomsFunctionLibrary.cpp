// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsFunctionLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UBackroomsFunctionLibrary::FadeCameraToBlack(APlayerController* PlayerController, float Duration)
{
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		// From 0.0 to 1.0, black color, hold when finished
		PlayerController->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, Duration, FLinearColor::Black, false, true);
	}
}

void UBackroomsFunctionLibrary::FadeCameraFromBlack(APlayerController* PlayerController, float Duration)
{
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		// From 1.0 to 0.0, black color, do not hold
		PlayerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, Duration, FLinearColor::Black, false, false);
	}
}

void UBackroomsFunctionLibrary::TransitionToLevelWithFade(const UObject* WorldContextObject, FName LevelName, float FadeDuration)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	FadeCameraToBlack(PC, FadeDuration);

	FTimerHandle TransitionTimerHandle;
	World->GetTimerManager().SetTimer(TransitionTimerHandle, [WorldContextObject, LevelName]()
	{
		UGameplayStatics::OpenLevel(WorldContextObject, LevelName);
	}, FadeDuration, false);
}
