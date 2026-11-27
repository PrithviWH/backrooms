// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsPlayerController.h"
#include "BackroomsAssetManager.h"
#include "BackroomsGameInstance.h"
#include "BackroomsPauseMenu.h"
#include "BackroomsSaveGame.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"

ABackroomsPlayerController::ABackroomsPlayerController()
	: DefaultMappingContext(nullptr)
	, PauseAction(nullptr)
	, PauseMenuClass(nullptr)
	, PauseMenuInstance(nullptr)
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;      // false during gameplay to prevent mouse delta interception
	bEnableMouseOverEvents = false;  // false during gameplay to prevent mouse delta interception

	PauseMenuClass = UBackroomsPauseMenu::StaticClass();
}

void ABackroomsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Enforce clean gameplay input: no mouse cursor, no click/hover UI hit-testing
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	// IMC registration is handled solely by ABackroomsCharacter to prevent duplicate conflict
	// Apply persistent saved settings via GameInstance to eliminate redundant disk I/O
	UBackroomsSaveGame* Settings = nullptr;
	if (UBackroomsGameInstance* GI = GetGameInstance<UBackroomsGameInstance>())
	{
		Settings = GI->GetCachedSettings();
	}
	else
	{
		Settings = UBackroomsSaveGame::LoadOrCreateSettings();
	}

	if (Settings)
	{
		UBackroomsSaveGame::ApplyAllSettings(this, Settings);
	}
}

void ABackroomsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!PauseAction)
	{
		PauseAction = UBackroomsAssetManager::Get().PauseAction.LoadSynchronous();
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PauseAction)
		{
			PauseAction->bTriggerWhenPaused = true;
			EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Triggered, this, &ABackroomsPlayerController::TogglePauseMenu);
		}
	}
}

void ABackroomsPlayerController::OpenPauseMenu()
{
	// Enable mouse events only when UI is active
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	if (!PauseMenuInstance || !IsValid(PauseMenuInstance))
	{
		TSubclassOf<UUserWidget> ClassToUse = PauseMenuClass ? PauseMenuClass : TSubclassOf<UUserWidget>(UBackroomsPauseMenu::StaticClass());
		PauseMenuInstance = CreateWidget<UUserWidget>(this, ClassToUse);
	}

	if (PauseMenuInstance && IsValid(PauseMenuInstance))
	{
		PauseMenuInstance->SetVisibility(ESlateVisibility::Visible);
		if (!PauseMenuInstance->IsInViewport())
		{
			PauseMenuInstance->AddToViewport(100);
		}
	}

	SetShowMouseCursor(true);

	FInputModeGameAndUI InputMode;
	if (PauseMenuInstance && IsValid(PauseMenuInstance))
	{
		InputMode.SetWidgetToFocus(PauseMenuInstance->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	SetPause(true);
}

void ABackroomsPlayerController::ClosePauseMenu()
{
	// Disable mouse UI events when returning to gameplay
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	if (PauseMenuInstance && IsValid(PauseMenuInstance))
	{
		if (UBackroomsPauseMenu* PauseMenu = Cast<UBackroomsPauseMenu>(PauseMenuInstance))
		{
			if (PauseMenu->SettingsMenuInstance && PauseMenu->SettingsMenuInstance->IsInViewport())
			{
				PauseMenu->SettingsMenuInstance->RemoveFromParent();
				PauseMenu->SettingsMenuInstance = nullptr;
			}
		}

		PauseMenuInstance->RemoveFromParent();
	}
	PauseMenuInstance = nullptr;

	SetShowMouseCursor(false);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	SetPause(false);
}

void ABackroomsPlayerController::TogglePauseMenu()
{
	if (PauseMenuInstance && IsValid(PauseMenuInstance) && PauseMenuInstance->IsInViewport())
	{
		ClosePauseMenu();
	}
	else
	{
		if (PauseMenuInstance && !IsValid(PauseMenuInstance))
		{
			PauseMenuInstance = nullptr;
		}
		OpenPauseMenu();
	}
}
