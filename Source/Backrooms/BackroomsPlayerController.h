// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BackroomsPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;

/**
 * Authoritative Player Controller for Backrooms.
 * Handles Enhanced Input Mapping Context initialization, IA_Pause handling, and Pause Menu in native C++.
 */
UCLASS()
class BACKROOMS_API ABackroomsPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABackroomsPlayerController();

	/** Default Input Mapping Context to apply on begin play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Pause Input Action (Escape / Gamepad Start) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> PauseAction;

	/** Widget class to instantiate for the Pause Menu */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuClass;

	/** Active Pause Menu widget instance */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> PauseMenuInstance;

	/** Toggle the Pause Menu (opens if closed, closes if open) */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void TogglePauseMenu();

	/** Open the Pause Menu, pause the game, and show mouse cursor */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void OpenPauseMenu();

	/** Close the Pause Menu, unpause the game, and restore game-only input mode */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void ClosePauseMenu();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
};
