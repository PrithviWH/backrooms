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
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"

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

	// Footstep audio defaults
	WalkFootstepVolume = 0.4f;
	SprintFootstepVolume = 0.7f;
	MinFootstepPitch = 0.95f;
	MaxFootstepPitch = 1.05f;
	BaseStepDistance = 150.0f;
	SprintStepDistance = 180.0f;
	FootstepDistanceAccumulator = 0.0f;
	LastCarpetSoundIndex = -1;
	LastDefaultSoundIndex = -1;

	// Populate default assets from project content
	static ConstructorHelpers::FObjectFinder<UPhysicalMaterial> CarpetMatObj(TEXT("/Game/Backrooms/Materials/PM_Carpet"));
	if (CarpetMatObj.Succeeded())
	{
		CarpetPhysicalMaterial = CarpetMatObj.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> Step1Obj(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step1"));
	if (Step1Obj.Succeeded()) { CarpetFootstepSounds.Add(Step1Obj.Object); }

	static ConstructorHelpers::FObjectFinder<USoundBase> Step2Obj(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step2"));
	if (Step2Obj.Succeeded()) { CarpetFootstepSounds.Add(Step2Obj.Object); }

	static ConstructorHelpers::FObjectFinder<USoundBase> Step3Obj(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step3"));
	if (Step3Obj.Succeeded()) { CarpetFootstepSounds.Add(Step3Obj.Object); }

	static ConstructorHelpers::FObjectFinder<USoundBase> Step4Obj(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step4"));
	if (Step4Obj.Succeeded()) { CarpetFootstepSounds.Add(Step4Obj.Object); }

	static ConstructorHelpers::FObjectFinder<USoundBase> Step5Obj(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step5"));
	if (Step5Obj.Succeeded()) { CarpetFootstepSounds.Add(Step5Obj.Object); }

	static ConstructorHelpers::FObjectFinder<USoundBase> DefaultStepObj(TEXT("/Game/Backrooms/Audio/Foot-steps/freeeverythingxx-walking-on-concrete-ver-2-268513"));
	if (DefaultStepObj.Succeeded()) { DefaultFootstepSounds.Add(DefaultStepObj.Object); }
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

void ABackroomsCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateFootsteps(DeltaTime);
}

void ABackroomsCharacter::UpdateFootsteps(float DeltaTime)
{
	if (!GetCharacterMovement() || !GetCharacterMovement()->IsMovingOnGround())
	{
		FootstepDistanceAccumulator = 0.0f;
		return;
	}

	const float Speed2D = GetVelocity().Size2D();
	if (Speed2D < 15.0f)
	{
		FootstepDistanceAccumulator = 0.0f;
		return;
	}

	FootstepDistanceAccumulator += Speed2D * DeltaTime;

	const float SpeedAlpha = FMath::Clamp((Speed2D - WalkSpeed) / FMath::Max(1.0f, SprintSpeed - WalkSpeed), 0.0f, 1.0f);
	const float CurrentStepDistance = FMath::Lerp(BaseStepDistance, SprintStepDistance, SpeedAlpha);

	if (FootstepDistanceAccumulator >= CurrentStepDistance)
	{
		FootstepDistanceAccumulator = 0.0f;
		TriggerFootstep();
	}
}

void ABackroomsCharacter::TriggerFootstep()
{
	const float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.0f;
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, CapsuleHalfHeight + 35.0f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FootstepTrace), false, this);
	QueryParams.bReturnPhysicalMaterial = true;

	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	if (!bHit)
	{
		return;
	}

	UPhysicalMaterial* HitPhysMat = HitResult.PhysMaterial.Get();
	const bool bIsCarpet = (HitPhysMat != nullptr) && (
		HitPhysMat == CarpetPhysicalMaterial ||
		HitPhysMat->SurfaceType == SurfaceType1 ||
		HitPhysMat->GetName().Contains(TEXT("Carpet"))
	);

	const TArray<TObjectPtr<USoundBase>>& CandidateSounds = (bIsCarpet && CarpetFootstepSounds.Num() > 0)
		? CarpetFootstepSounds
		: DefaultFootstepSounds;

	if (CandidateSounds.Num() == 0)
	{
		return;
	}

	int32 SelectedIndex = 0;
	int32& LastIndex = bIsCarpet ? LastCarpetSoundIndex : LastDefaultSoundIndex;
	if (CandidateSounds.Num() > 1)
	{
		do
		{
			SelectedIndex = FMath::RandRange(0, CandidateSounds.Num() - 1);
		} while (SelectedIndex == LastIndex);
	}
	LastIndex = SelectedIndex;

	USoundBase* SoundToPlay = CandidateSounds[SelectedIndex];
	if (!SoundToPlay)
	{
		return;
	}

	const float Speed2D = GetVelocity().Size2D();
	const float SpeedAlpha = FMath::Clamp((Speed2D - WalkSpeed) / FMath::Max(1.0f, SprintSpeed - WalkSpeed), 0.0f, 1.0f);
	const float Volume = FMath::Lerp(WalkFootstepVolume, SprintFootstepVolume, SpeedAlpha);
	const float Pitch = FMath::FRandRange(MinFootstepPitch, MaxFootstepPitch);

	UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, HitResult.ImpactPoint, Volume, Pitch);

	OnFootstepPlayed(SoundToPlay, HitResult.ImpactPoint, Volume, Pitch, bIsCarpet);
}

