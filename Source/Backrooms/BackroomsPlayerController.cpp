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
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	// Load default mapping context if not already set
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCFinder(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	if (IMCFinder.Succeeded())
	{
		DefaultMappingContext = IMCFinder.Object;
	}

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

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Apply all persistent saved settings (Audio, Scalability, Controls)
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
