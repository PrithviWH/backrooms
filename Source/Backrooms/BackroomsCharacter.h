// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "BackroomsCharacter.generated.h"

class UCameraComponent;
class USpotLightComponent;
class UInputMappingContext;
class UInputAction;
class UPhysicalMaterial;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSprintStateChanged, bool, bIsSprinting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFlashlightToggled, bool, bIsToggled);

/**
 * Authoritative Character for Backrooms.
 * Implements first-person camera, attached flashlight, ungated free sprint mechanics,
 * smooth camera FOV speed dynamics, and Enhanced Input handling in native C++.
 */
UCLASS()
class BACKROOMS_API ABackroomsCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABackroomsCharacter();

	/** First person camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** Flashlight spot light component attached to camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<USpotLightComponent> FlashlightComponent;

	/** Base walking speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed;

	/** Sprinting speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed;

	/** Default camera field of view */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "60.0", ClampMax = "120.0"))
	float DefaultFOV;

	/** Target camera field of view when sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "60.0", ClampMax = "120.0"))
	float SprintFOV;

	/** Whether the player is currently requesting sprint */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bWantsToSprint;

	/** Delegate fired when sprint state changes */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSprintStateChanged OnSprintStateChanged;

	/** Delegate fired when flashlight is toggled */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnFlashlightToggled OnFlashlightToggled;

	/** Default Enhanced Input Mapping Context */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	/** Flashlight Toggle Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> FlashlightAction;

	/** Physical Material associated with carpet flooring */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	TObjectPtr<UPhysicalMaterial> CarpetPhysicalMaterial;

	/** Footstep sounds played when walking or running on carpet */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	TArray<TObjectPtr<USoundBase>> CarpetFootstepSounds;

	/** Default footstep sounds played on non-carpet surfaces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps")
	TArray<TObjectPtr<USoundBase>> DefaultFootstepSounds;

	/** Footstep volume multiplier during normal walking (0.3 - 0.5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float WalkFootstepVolume;

	/** Footstep volume multiplier during sprinting (0.6 - 0.8) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SprintFootstepVolume;

	/** Minimum random pitch multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float MinFootstepPitch;

	/** Maximum random pitch multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float MaxFootstepPitch;

	/** Distance traveled in cm required to trigger a footstep at walk speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps", meta = (ClampMin = "50.0", ClampMax = "400.0"))
	float BaseStepDistance;

	/** Distance traveled in cm required to trigger a footstep at sprint speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footsteps", meta = (ClampMin = "50.0", ClampMax = "400.0"))
	float SprintStepDistance;

	UFUNCTION(BlueprintCallable, Category = "Footsteps")
	void TriggerFootstep();

	UFUNCTION(BlueprintImplementableEvent, Category = "Footsteps")
	void OnFootstepPlayed(USoundBase* Sound, FVector Location, float Volume, float Pitch, bool bIsCarpet);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSprint();

	UFUNCTION(BlueprintCallable, Category = "Flashlight")
	void ToggleFlashlight();

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool GetWantsToSprint() const { return bWantsToSprint; }

	/** Mouse sensitivity multiplier (0.1 to 3.0, default 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	float MouseSensitivity;

	/** Whether vertical look axis is inverted */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	bool bInvertY;

	UFUNCTION(BlueprintCallable, Category = "Controls")
	void SetMouseSensitivity(float InSensitivity) { MouseSensitivity = FMath::Clamp(InSensitivity, 0.1f, 3.0f); }

	UFUNCTION(BlueprintCallable, Category = "Controls")
	void SetInvertY(bool bInInvertY) { bInvertY = bInInvertY; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void UpdateFootsteps(float DeltaTime);

	float FootstepDistanceAccumulator;
	int32 LastCarpetSoundIndex;
	int32 LastDefaultSoundIndex;
};
