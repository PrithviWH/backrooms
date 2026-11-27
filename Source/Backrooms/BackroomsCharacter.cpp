// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsCharacter.h"
#include "BackroomsAssetManager.h"
#include "BackroomsGameInstance.h"
#include "BackroomsSaveGame.h"
#include "BackroomsFootstepDataAsset.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "DrawDebugHelpers.h"

ABackroomsCharacter::ABackroomsCharacter()
	: CameraProbeRadius(12.0f)
	, CameraMinDistance(10.0f)
	, CameraSmoothSpeed(10.0f)
	, DefaultCameraOffset(FVector(15.0f, 0.0f, 0.0f))
	, bEnableCameraCollision(true)
	, bDrawDebugCameraCollision(false)
	, WalkSpeed(300.0f)
	, SprintSpeed(600.0f)
	, DefaultFOV(90.0f)
	, SprintFOV(98.0f)
	, bWantsToSprint(false)
	, SprintTime(0.0f)
	, SprintMeter(1.0f)
	, WalkFootstepVolume(0.4f)
	, SprintFootstepVolume(0.7f)
	, MinFootstepPitch(0.95f)
	, MaxFootstepPitch(1.05f)
	, BaseStepDistance(150.0f)
	, SprintStepDistance(180.0f)
	, MouseSensitivity(1.0f)
	, bInvertY(false)
	, FootstepDistanceAccumulator(0.0f)
	, LastCarpetSoundIndex(-1)
	, LastDefaultSoundIndex(-1)
	, DesiredCameraLocation(FVector::ZeroVector)
	, CurrentCameraOffset(FVector(15.0f, 0.0f, 0.0f))
	, BaseHeadRelativeLocation(FVector(0.0f, 0.0f, 64.0f))
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// Create First Person Camera Component
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(BaseHeadRelativeLocation + DefaultCameraOffset); // Position the camera at eye level + forward offset
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
	FootstepData = nullptr;
}

void ABackroomsCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Skip asset loading during Class Default Object (CDO) construction to prevent startup blocking
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	// Populate from primary footstep data asset if assigned
	if (FootstepData)
	{
		if (!CarpetPhysicalMaterial && FootstepData->CarpetPhysicalMaterial)
		{
			CarpetPhysicalMaterial = FootstepData->CarpetPhysicalMaterial;
		}
		if (CarpetFootstepSounds.Num() == 0)
		{
			CarpetFootstepSounds = FootstepData->CarpetFootstepSounds;
		}
		if (DefaultFootstepSounds.Num() == 0)
		{
			DefaultFootstepSounds = FootstepData->DefaultFootstepSounds;
		}
	}

	// Fallback lazy load physical material and sound assets from Asset Manager
	UBackroomsAssetManager& AssetManager = UBackroomsAssetManager::Get();

	if (!CarpetPhysicalMaterial)
	{
		CarpetPhysicalMaterial = AssetManager.CarpetPhysicalMaterial.LoadSynchronous();
	}
	if (CarpetFootstepSounds.Num() == 0)
	{
		for (const TSoftObjectPtr<USoundBase>& SoftSnd : AssetManager.CarpetFootstepSounds)
		{
			if (USoundBase* Snd = SoftSnd.LoadSynchronous())
			{
				CarpetFootstepSounds.Add(Snd);
			}
		}
	}
	if (DefaultFootstepSounds.Num() == 0)
	{
		if (USoundBase* DefSnd = AssetManager.ConcreteFootstepSound.LoadSynchronous())
		{
			DefaultFootstepSounds.Add(DefSnd);
		}
	}

	// Fallback lazy load Input Mapping Contexts
	if (!DefaultMappingContext)
	{
		DefaultMappingContext = AssetManager.DefaultMappingContext.LoadSynchronous();
	}
	if (!MouseLookMappingContext)
	{
		MouseLookMappingContext = AssetManager.MouseLookMappingContext.LoadSynchronous();
	}

	// Fallback lazy load Input Actions
	if (!MoveAction)
	{
		MoveAction = AssetManager.MoveAction.LoadSynchronous();
	}
	if (!LookAction)
	{
		LookAction = AssetManager.LookAction.LoadSynchronous();
	}
	if (!MouseLookAction)
	{
		MouseLookAction = AssetManager.MouseLookAction.LoadSynchronous();
	}
	if (!JumpAction)
	{
		JumpAction = AssetManager.JumpAction.LoadSynchronous();
	}
	if (!SprintAction)
	{
		SprintAction = AssetManager.SprintAction.LoadSynchronous();
	}
}

void ABackroomsCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Load controls preferences from cached GameInstance settings (prevents disk I/O race conditions)
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
		MouseSensitivity = Settings->MouseSensitivity;
		bInvertY = Settings->bInvertY;
	}

	// Add Input Mapping Contexts if player controller is available
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
			if (MouseLookMappingContext)
			{
				Subsystem->AddMappingContext(MouseLookMappingContext, 1);
			}
		}
	}
}

void ABackroomsCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// Ensure MappingContexts are valid
	if (!DefaultMappingContext)
	{
		DefaultMappingContext = UBackroomsAssetManager::Get().DefaultMappingContext.LoadSynchronous();
	}
	if (!MouseLookMappingContext)
	{
		MouseLookMappingContext = UBackroomsAssetManager::Get().MouseLookMappingContext.LoadSynchronous();
	}

	// Add Input Mapping Context to local player subsystem upon possession/restart
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
			if (MouseLookMappingContext)
			{
				Subsystem->AddMappingContext(MouseLookMappingContext, 1);
			}
		}
	}
}

void ABackroomsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Fallback load input actions if not set on CDO/instance
	UBackroomsAssetManager& AssetManager = UBackroomsAssetManager::Get();
	if (!MoveAction)
	{
		MoveAction = AssetManager.MoveAction.LoadSynchronous();
	}
	if (!LookAction)
	{
		LookAction = AssetManager.LookAction.LoadSynchronous();
	}
	if (!MouseLookAction)
	{
		MouseLookAction = AssetManager.MouseLookAction.LoadSynchronous();
	}
	if (!JumpAction)
	{
		JumpAction = AssetManager.JumpAction.LoadSynchronous();
	}
	if (!SprintAction)
	{
		SprintAction = AssetManager.SprintAction.LoadSynchronous();
	}

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

		if (MouseLookAction && MouseLookAction != LookAction)
		{
			EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ABackroomsCharacter::Look);
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
		const float PitchMultiplier = bInvertY ? -1.0f : 1.0f;
		AddControllerYawInput(LookAxisVector.X * MouseSensitivity);
		AddControllerPitchInput(LookAxisVector.Y * MouseSensitivity * PitchMultiplier);
	}
}

void ABackroomsCharacter::StartSprint()
{
	bWantsToSprint = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	OnSprintStateChanged.Broadcast(true);
}

void ABackroomsCharacter::StopSprint()
{
	bWantsToSprint = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	OnSprintStateChanged.Broadcast(false);
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

void ABackroomsCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateFootsteps(DeltaTime);
	UpdateCameraCollision(DeltaTime);

	// Smooth sprint FOV transition
	if (FirstPersonCamera)
	{
		const bool bIsMoving = GetCharacterMovement() && GetCharacterMovement()->Velocity.SizeSquared() > 100.0f;
		const float TargetFOV = (bWantsToSprint && bIsMoving) ? SprintFOV : DefaultFOV;
		const float NewFOV = FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFOV, DeltaTime, 6.0f);
		FirstPersonCamera->SetFieldOfView(NewFOV);
	}
}

void ABackroomsCharacter::UpdateCameraCollision(float DeltaTime)
{
	if (!FirstPersonCamera)
	{
		return;
	}

	if (!bEnableCameraCollision)
	{
		CurrentCameraOffset = FMath::VInterpTo(CurrentCameraOffset, DefaultCameraOffset, DeltaTime, CameraSmoothSpeed);
		FirstPersonCamera->SetRelativeLocation(BaseHeadRelativeLocation + CurrentCameraOffset);
		return;
	}

	// Early exit if camera is already at rest at default offset and character is stationary
	if (CurrentCameraOffset.Equals(DefaultCameraOffset, 0.5f) && GetVelocity().SizeSquared() < 100.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FTransform& CapsuleTransform = GetCapsuleComponent() ? GetCapsuleComponent()->GetComponentTransform() : GetActorTransform();

	// Determine head/pivot location in world space
	FVector HeadLocation;
	if (GetMesh() && GetMesh()->DoesSocketExist(TEXT("head")))
	{
		HeadLocation = GetMesh()->GetSocketLocation(TEXT("head"));
	}
	else
	{
		HeadLocation = CapsuleTransform.TransformPosition(BaseHeadRelativeLocation);
	}

	// Compute desired camera position in world space
	DesiredCameraLocation = CapsuleTransform.TransformPosition(BaseHeadRelativeLocation + DefaultCameraOffset);

	// Prepare collision query ignoring self and own components
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CameraCollisionTrace), false, this);
	QueryParams.AddIgnoredActor(this);
	if (GetMesh())
	{
		QueryParams.AddIgnoredComponent(GetMesh());
	}
	if (GetCapsuleComponent())
	{
		QueryParams.AddIgnoredComponent(GetCapsuleComponent());
	}

	const FCollisionShape ProbeShape = FCollisionShape::MakeSphere(CameraProbeRadius);
	FHitResult HitResult;

	// Sweep sphere along Camera channel from Head to Desired location
	const bool bHit = World->SweepSingleByChannel(
		HitResult,
		HeadLocation,
		DesiredCameraLocation,
		FQuat::Identity,
		ECC_Camera,
		ProbeShape,
		QueryParams
	);

	FVector TargetOffset = DefaultCameraOffset;

	if (bHit)
	{
		const float DesiredDist = DefaultCameraOffset.Size();
		const FVector Direction = (DesiredDist > KINDA_SMALL_NUMBER) ? DefaultCameraOffset.GetSafeNormal() : FVector::ForwardVector;

		float SafeDistance = HitResult.Distance;
		if (HitResult.bStartPenetrating)
		{
			SafeDistance = 0.0f;
		}
		else
		{
			// Safe distance clamped between CameraMinDistance and desired distance,
			// while allowing pull-back to hit distance if penetration occurs inside MinDistance
			SafeDistance = FMath::Clamp(SafeDistance, FMath::Min(CameraMinDistance, SafeDistance), DesiredDist);
		}

		TargetOffset = Direction * SafeDistance;
	}

	// Smoothly interpolate current camera offset to target offset
	CurrentCameraOffset = FMath::VInterpTo(CurrentCameraOffset, TargetOffset, DeltaTime, CameraSmoothSpeed);
	FirstPersonCamera->SetRelativeLocation(BaseHeadRelativeLocation + CurrentCameraOffset);

#if !UE_BUILD_SHIPPING
	if (bDrawDebugCameraCollision)
	{
		DrawDebugSphere(World, HeadLocation, CameraProbeRadius, 8, FColor::Green, false, -1.0f, 0, 0.5f);
		DrawDebugSphere(World, DesiredCameraLocation, CameraProbeRadius, 8, bHit ? FColor::Red : FColor::Cyan, false, -1.0f, 0, 0.5f);
		DrawDebugLine(World, HeadLocation, DesiredCameraLocation, bHit ? FColor::Red : FColor::Green, false, -1.0f, 0, 1.0f);
		if (bHit)
		{
			DrawDebugPoint(World, HitResult.ImpactPoint, 10.0f, FColor::Yellow, false, -1.0f, 0);
		}
	}
#endif
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

