// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsPlayerController.h"
#include "BackroomsPauseMenu.h"
#include "BackroomsSaveGame.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"

ABackroomsPlayerController::ABackroomsPlayerController()
	: DefaultMappingContext(nullptr)
	, PauseAction(nullptr)
	, PauseMenuClass(nullptr)
	, PauseMenuInstance(nullptr)
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;      // false during gameplay to prevent mouse delta interception
	bEnableMouseOverEvents = false;  // false during gameplay to prevent mouse delta interception

	// Load IA_Pause
	static ConstructorHelpers::FObjectFinder<UInputAction> PauseFinder(TEXT("/Game/Input/Actions/IA_Pause.IA_Pause"));
	if (PauseFinder.Succeeded())
	{
		PauseAction = PauseFinder.Object;
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UInputAction> PauseFinderFallback(TEXT("/Game/Input/IA_Pause.IA_Pause"));
		if (PauseFinderFallback.Succeeded())
		{
			PauseAction = PauseFinderFallback.Object;
		}
	}

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
	// Apply persistent saved settings here
	if (UBackroomsSaveGame* Settings = UBackroomsSaveGame::LoadOrCreateSettings())
	{
		UBackroomsSaveGame::ApplyAllSettings(this, Settings);
	}
}

void ABackroomsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PauseAction)
		{
			EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Triggered, this, &ABackroomsPlayerController::TogglePauseMenu);
		}
	}
}

void ABackroomsPlayerController::OpenPauseMenu()
{
	// Enable mouse events only when UI is active
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	if (!PauseMenuInstance)
	{
		TSubclassOf<UUserWidget> ClassToUse = PauseMenuClass ? PauseMenuClass : TSubclassOf<UUserWidget>(UBackroomsPauseMenu::StaticClass());
		PauseMenuInstance = CreateWidget<UUserWidget>(this, ClassToUse);
	}

	if (PauseMenuInstance && !PauseMenuInstance->IsInViewport())
	{
		PauseMenuInstance->AddToViewport(100);
	}

	SetShowMouseCursor(true);

	FInputModeGameAndUI InputMode;
	if (PauseMenuInstance)
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

	if (PauseMenuInstance)
	{
		PauseMenuInstance->RemoveFromParent();
		PauseMenuInstance = nullptr;
	}

	SetShowMouseCursor(false);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	SetPause(false);
}

void ABackroomsPlayerController::TogglePauseMenu()
{
	if (PauseMenuInstance && PauseMenuInstance->IsInViewport())
	{
		ClosePauseMenu();
	}
	else
	{
		OpenPauseMenu();
	}
}
