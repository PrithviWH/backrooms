// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

ABackroomsPlayerController::ABackroomsPlayerController()
	: DefaultMappingContext(nullptr)
{
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
}
