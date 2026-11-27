// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "TimerManager.h"

ABackroomsCharacter::ABackroomsCharacter()
	: SprintMeter(5.0f)
	, SprintTime(5.0f)
	, SprintFixedTickTime(0.05f)
	, WalkSpeed(300.0f)
	, SprintSpeed(600.0f)
	, RecoveringWalkSpeed(150.0f)
	, bWantsToSprint(false)
	, bRecovering(false)
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// Create First Person Camera Component
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f)); // Position the camera at eye level
	FirstPersonCamera->bUsePawnControlRotation = true;

	// Create Flashlight Component attached to Camera
	FlashlightComponent = CreateDefaultSubobject<USpotLightComponent>(TEXT("FlashlightComponent"));
	FlashlightComponent->SetupAttachment(FirstPersonCamera);
	FlashlightComponent->SetRelativeLocation(FVector(10.0f, 0.0f, -5.0f));
	FlashlightComponent->Intensity = 1800.0f;
	FlashlightComponent->InnerConeAngle = 0.0f;
	FlashlightComponent->OuterConeAngle = 38.0f;
	FlashlightComponent->AttenuationRadius = 2400.0f;
	FlashlightComponent->SourceRadius = 8.0f;
	FlashlightComponent->SoftSourceRadius = 15.0f;
	FlashlightComponent->SetVisibility(false);

	// Configure movement defaults
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
}

void ABackroomsCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Context if player controller is available
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// Start looping sprint/stamina timer
	GetWorld()->GetTimerManager().SetTimer(SprintTimerHandle, this, &ABackroomsCharacter::SprintFixedTick, SprintFixedTickTime, true);
}

void ABackroomsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}

		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABackroomsCharacter::Move);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABackroomsCharacter::Look);
		}

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ABackroomsCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABackroomsCharacter::StopSprint);
		}

		if (FlashlightAction)
		{
			EnhancedInputComponent->BindAction(FlashlightAction, ETriggerEvent::Started, this, &ABackroomsCharacter::ToggleFlashlight);
		}
	}
}

void ABackroomsCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ABackroomsCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ABackroomsCharacter::StartSprint()
{
	bWantsToSprint = true;
	if (!bRecovering)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		OnSprintStateChanged.Broadcast(true);
	}
}

void ABackroomsCharacter::StopSprint()
{
	bWantsToSprint = false;
	if (!bRecovering)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		OnSprintStateChanged.Broadcast(false);
	}
}

void ABackroomsCharacter::ToggleFlashlight()
{
	if (FlashlightComponent)
	{
		const bool bNewVisibility = !FlashlightComponent->IsVisible();
		FlashlightComponent->SetVisibility(bNewVisibility);
		OnFlashlightToggled.Broadcast(bNewVisibility);
	}
}

void ABackroomsCharacter::SprintFixedTick()
{
	if (bWantsToSprint && !bRecovering)
	{
		if (SprintMeter > 0.0f)
		{
			SprintMeter = FMath::Max(0.0f, SprintMeter - SprintFixedTickTime);
			if (SprintMeter <= 0.0f)
			{
				bRecovering = true;
				GetCharacterMovement()->MaxWalkSpeed = RecoveringWalkSpeed;
				OnSprintStateChanged.Broadcast(false);
			}
		}
	}
	else
	{
		if (SprintMeter < SprintTime)
		{
			SprintMeter = FMath::Min(SprintTime, SprintMeter + SprintFixedTickTime);
			if (SprintMeter >= SprintTime)
			{
				bRecovering = false;
				GetCharacterMovement()->MaxWalkSpeed = bWantsToSprint ? SprintSpeed : WalkSpeed;
				OnSprintStateChanged.Broadcast(bWantsToSprint);
			}
		}
	}

	const double NormalizedRatio = (SprintTime > 0.0f) ? static_cast<double>(SprintMeter / SprintTime) : 0.0;
	OnSprintMeterUpdated.Broadcast(NormalizedRatio);
}
