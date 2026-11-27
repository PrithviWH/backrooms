// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BackroomsPlayerController.generated.h"

class UInputMappingContext;

/**
 * Authoritative Player Controller for Backrooms.
 * Handles Enhanced Input Mapping Context initialization in native C++.
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

protected:
	virtual void BeginPlay() override;
};
