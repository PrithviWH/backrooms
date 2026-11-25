// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BackroomsFunctionLibrary.generated.h"

class APlayerController;

/**
 * Blueprint function library for Backrooms gameplay helpers:
 * Camera fades, level transitions, and presentation utilities.
 */
UCLASS()
class BACKROOMS_API UBackroomsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Performs camera fade to black on the specified player camera manager (holds at black) */
	UFUNCTION(BlueprintCallable, Category = "Backrooms|Camera")
	static void FadeCameraToBlack(APlayerController* PlayerController, float Duration = 1.0f);

	/** Performs camera fade from black on the specified player camera manager */
	UFUNCTION(BlueprintCallable, Category = "Backrooms|Camera")
	static void FadeCameraFromBlack(APlayerController* PlayerController, float Duration = 1.0f);

	/** Initiates a level transition with camera fade-to-black before loading */
	UFUNCTION(BlueprintCallable, Category = "Backrooms|Transition", meta = (WorldContext = "WorldContextObject"))
	static void TransitionToLevelWithFade(const UObject* WorldContextObject, FName LevelName, float FadeDuration = 1.0f);
};
