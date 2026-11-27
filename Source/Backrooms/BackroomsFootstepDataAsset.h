// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BackroomsFootstepDataAsset.generated.h"

class USoundBase;
class UPhysicalMaterial;

/**
 * Primary Data Asset grouping footstep sound collections and physical materials.
 * Avoids hardcoded asset paths and eliminates constructor blocking during CDO construction.
 */
UCLASS(BlueprintType)
class BACKROOMS_API UFootstepDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Carpet physical material reference */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	TObjectPtr<UPhysicalMaterial> CarpetPhysicalMaterial;

	/** Carpet footstep sound effects (soft Berber thuds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	TArray<TObjectPtr<USoundBase>> CarpetFootstepSounds;

	/** Default concrete footstep sound effects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	TArray<TObjectPtr<USoundBase>> DefaultFootstepSounds;

	/** Heavy monster footstep sound effects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	TArray<TObjectPtr<USoundBase>> HeavyFootstepSounds;
};
