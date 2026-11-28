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
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "BackroomsEntity.h"
#include "BackroomsAtmosphereManager.h"
#include "Camera/PlayerCameraManager.h"

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
	, MouseSensitivity(0.20f)
	, bInvertY(false)
	, SightStingerSound(nullptr)
	, bPlayerSeesEntity(false)
	, bHasTriggeredSightAudio(false)
	, CurrentGazeGlitchFactor(0.0f)
	, bIsUnderAttack(false)
	, FootstepDistanceAccumulator(0.0f)
	, LastCarpetSoundIndex(-1)
	, LastDefaultSoundIndex(-1)
	, DesiredCameraLocation(FVector::ZeroVector)
	, CurrentCameraOffset(FVector(15.0f, 0.0f, 0.0f))
	, BaseHeadRelativeLocation(FVector(0.0f, 0.0f, 64.0f))
	, RespawnLocation(FVector::ZeroVector)
	, RespawnRotation(FRotator::ZeroRotator)
	, GazeTremorTime(0.0f)
	, FlashlightBaseIntensity(1800.0f)
	, LostSightCooldownTimer(0.0f)
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

	// Horror Proximity Heartbeat & Panic Audio (2D non-spatialized direct to player ears)
	HeartbeatAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("HeartbeatAudioComponent"));
	HeartbeatAudioComponent->SetupAttachment(RootComponent);
	HeartbeatAudioComponent->bAutoActivate = false;
	HeartbeatAudioComponent->bStopWhenOwnerDestroyed = true;
	HeartbeatAudioComponent->bIsUISound = true;

	PanicBreathingAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("PanicBreathingAudioComponent"));
	PanicBreathingAudioComponent->SetupAttachment(RootComponent);
	PanicBreathingAudioComponent->bAutoActivate = false;
	PanicBreathingAudioComponent->bStopWhenOwnerDestroyed = true;
	PanicBreathingAudioComponent->bIsUISound = true;

	MaxHeartbeatDistance = 2500.0f;
	MinHeartbeatDistance = 400.0f;
	MaxHeartbeatVolume = 0.95f;
	CurrentPanicFactor = 0.0f;
	CurrentHeartbeatVolume = 0.0f;
	CurrentBreathingVolume = 0.0f;
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

	// Fallback load Heartbeat and Panic sounds
	if (!HeartbeatSound)
	{
		HeartbeatSound = AssetManager.HeartbeatSound.LoadSynchronous();
	}
	if (!PanicBreathingSound)
	{
		PanicBreathingSound = AssetManager.OutOfBreathSound.LoadSynchronous();
	}
	if (!SightStingerSound)
	{
		SightStingerSound = AssetManager.EntitySightStingerSound.LoadSynchronous();
	}
}

void ABackroomsCharacter::BeginPlay()
{
	Super::BeginPlay();

	RespawnLocation = GetActorLocation();
	RespawnRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
	FlashlightBaseIntensity = FlashlightComponent ? FlashlightComponent->Intensity : 1800.0f;

	// Initialize horror audio components with assigned sounds
	if (HeartbeatAudioComponent && HeartbeatSound)
	{
		HeartbeatAudioComponent->SetSound(HeartbeatSound);
	}
	if (PanicBreathingAudioComponent && PanicBreathingSound)
	{
		PanicBreathingAudioComponent->SetSound(PanicBreathingSound);
	}

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

	// Ensure Atmosphere Manager is active in the world for immersive liminal horror audio & location cues
	if (GetWorld() && !UGameplayStatics::GetActorOfClass(GetWorld(), ABackroomsAtmosphereManager::StaticClass()))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ABackroomsAtmosphereManager>(ABackroomsAtmosphereManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
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
	UpdateHeartbeat(DeltaTime);
	UpdateGazeDistortion(DeltaTime);

	// Smooth sprint FOV transition
	if (FirstPersonCamera)
	{
		const bool bIsMoving = GetCharacterMovement() && GetCharacterMovement()->Velocity.SizeSquared() > 100.0f;
		const float TargetFOV = (bWantsToSprint && bIsMoving) ? SprintFOV : DefaultFOV;
		const float NewFOV = FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFOV, DeltaTime, 6.0f);
		FirstPersonCamera->SetFieldOfView(NewFOV);
	}
}

void ABackroomsCharacter::UpdateHeartbeat(float DeltaTime)
{
	if (!HeartbeatAudioComponent)
	{
		return;
	}

	// Locate nearest monster entity
	ABackroomsEntity* NearestEntity = CachedNearestEntity.Get();
	if (!NearestEntity || !IsValid(NearestEntity))
	{
		float ClosestDistSq = MAX_FLT;
		const FVector MyLoc = GetActorLocation();

		for (TActorIterator<ABackroomsEntity> It(GetWorld()); It; ++It)
		{
			ABackroomsEntity* Entity = *It;
			if (Entity && IsValid(Entity))
			{
				const float DistSq = FVector::DistSquared(MyLoc, Entity->GetActorLocation());
				if (DistSq < ClosestDistSq)
				{
					ClosestDistSq = DistSq;
					NearestEntity = Entity;
				}
			}
		}
		CachedNearestEntity = NearestEntity;
	}

	float TargetPanic = 0.0f;
	bool bCanSeeMonsterInFront = false;

	if (NearestEntity && IsValid(NearestEntity) && !NearestEntity->IsActorBeingDestroyed() && !NearestEntity->IsHidden())
	{
		const FVector CamLoc = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : GetActorLocation();
		const FVector CamFwd = FirstPersonCamera ? FirstPersonCamera->GetForwardVector() : GetActorForwardVector();
		const FVector MonsterHead = NearestEntity->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
		const FVector ToMonster = MonsterHead - CamLoc;
		const float Distance = ToMonster.Size();
		const FVector DirToMonster = ToMonster / FMath::Max(Distance, 1.0f);
		const float LookAtDot = FVector::DotProduct(CamFwd, DirToMonster);

		// Forward view check: entity must be in front of the camera view cone (LookAtDot > 0.35f, ~140 deg FOV)
		if (LookAtDot > 0.35f)
		{
			// Line of sight raycast: ensure no wall or corridor obstacle blocks visibility
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerSightTrace), false, this);
			QueryParams.AddIgnoredActor(NearestEntity);
			FHitResult HitResult;
			const bool bBlocked = GetWorld()->LineTraceSingleByChannel(HitResult, CamLoc, MonsterHead, ECC_Visibility, QueryParams);

			if (!bBlocked)
			{
				bCanSeeMonsterInFront = true;

				// User requirement: play audio when player sees the entity in front of them, regardless of distance
				// Stinger audio cue triggers immediately when player first sees the entity in front of them
				if (!bHasTriggeredSightAudio)
				{
					bHasTriggeredSightAudio = true;
					if (SightStingerSound)
					{
						UGameplayStatics::PlaySound2D(this, SightStingerSound, 1.25f);
					}
				}

				// Panic / Heartbeat: begins at 0.45 upon sighting regardless of distance (up to 45m), scaling to 1.0 close up
				const float DistFactor = 1.0f - FMath::Clamp((Distance - 400.0f) / 4100.0f, 0.0f, 1.0f);
				TargetPanic = FMath::Lerp(0.45f, 1.0f, DistFactor);

				if (NearestEntity->CachedAIState == EBackroomsAIState::Chase)
				{
					TargetPanic = FMath::Max(TargetPanic, 0.85f);
				}
			}
		}
	}

	bPlayerSeesEntity = bCanSeeMonsterInFront;

	if (bCanSeeMonsterInFront)
	{
		LostSightCooldownTimer = 0.0f;
	}
	else
	{
		// Monster is behind player or behind a wall: ZERO panic audio, ZERO heartbeat
		TargetPanic = 0.0f;

		// Reset sight stinger after 4 seconds of not seeing the entity
		LostSightCooldownTimer += DeltaTime;
		if (LostSightCooldownTimer > 4.0f)
		{
			bHasTriggeredSightAudio = false;
		}
	}

	// Smoothly interpolate panic factor
	const float InterpSpeed = (TargetPanic > CurrentPanicFactor) ? 2.5f : 1.0f;
	CurrentPanicFactor = FMath::FInterpTo(CurrentPanicFactor, TargetPanic, DeltaTime, InterpSpeed);

	if (CurrentPanicFactor > 0.01f)
	{
		if (!HeartbeatAudioComponent->IsPlaying())
		{
			HeartbeatAudioComponent->Play();
		}

		const float TargetVol = CurrentPanicFactor * MaxHeartbeatVolume;
		const float TargetPitch = FMath::Lerp(0.95f, 1.35f, CurrentPanicFactor);

		CurrentHeartbeatVolume = FMath::FInterpTo(CurrentHeartbeatVolume, TargetVol, DeltaTime, 3.0f);
		HeartbeatAudioComponent->SetVolumeMultiplier(CurrentHeartbeatVolume);
		HeartbeatAudioComponent->SetPitchMultiplier(TargetPitch);

		// Panicked breathing layers in during extreme proximity (> 45% panic)
		if (PanicBreathingAudioComponent)
		{
			const float TargetBreathVol = FMath::Clamp((CurrentPanicFactor - 0.45f) / 0.55f, 0.0f, 1.0f) * 0.70f;
			if (TargetBreathVol > 0.05f)
			{
				if (!PanicBreathingAudioComponent->IsPlaying())
				{
					PanicBreathingAudioComponent->Play();
				}
				CurrentBreathingVolume = FMath::FInterpTo(CurrentBreathingVolume, TargetBreathVol, DeltaTime, 2.0f);
				PanicBreathingAudioComponent->SetVolumeMultiplier(CurrentBreathingVolume);
			}
			else if (PanicBreathingAudioComponent->IsPlaying())
			{
				CurrentBreathingVolume = FMath::FInterpTo(CurrentBreathingVolume, 0.0f, DeltaTime, 2.0f);
				PanicBreathingAudioComponent->SetVolumeMultiplier(CurrentBreathingVolume);
				if (CurrentBreathingVolume <= 0.01f)
				{
					PanicBreathingAudioComponent->Stop();
				}
			}
		}

		OnPanicFactorUpdated(CurrentPanicFactor);
	}
	else
	{
		if (HeartbeatAudioComponent->IsPlaying())
		{
			CurrentHeartbeatVolume = FMath::FInterpTo(CurrentHeartbeatVolume, 0.0f, DeltaTime, 2.0f);
			HeartbeatAudioComponent->SetVolumeMultiplier(CurrentHeartbeatVolume);
			if (CurrentHeartbeatVolume <= 0.01f)
			{
				HeartbeatAudioComponent->Stop();
			}
		}

		if (PanicBreathingAudioComponent && PanicBreathingAudioComponent->IsPlaying())
		{
			CurrentBreathingVolume = FMath::FInterpTo(CurrentBreathingVolume, 0.0f, DeltaTime, 2.0f);
			PanicBreathingAudioComponent->SetVolumeMultiplier(CurrentBreathingVolume);
			if (CurrentBreathingVolume <= 0.01f)
			{
				PanicBreathingAudioComponent->Stop();
			}
		}
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

void ABackroomsCharacter::UpdateGazeDistortion(float DeltaTime)
{
	if (bIsUnderAttack)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Locate nearest entity if cached pointer is invalid
	if (!CachedNearestEntity.IsValid())
	{
		for (TActorIterator<ABackroomsEntity> It(World); It; ++It)
		{
			if (!It->IsActorBeingDestroyed())
			{
				CachedNearestEntity = *It;
				break;
			}
		}
	}

	float TargetGlitch = 0.0f;

	if (CachedNearestEntity.IsValid() && !CachedNearestEntity->IsActorBeingDestroyed())
	{
		const FVector CamLoc = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : GetActorLocation();
		const FVector CamFwd = FirstPersonCamera ? FirstPersonCamera->GetForwardVector() : GetActorForwardVector();
		const FVector MonsterHeadLoc = CachedNearestEntity->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
		const FVector ToMonster = MonsterHeadLoc - CamLoc;
		const float Dist = ToMonster.Size();

		if (Dist < 1000.0f) // Glitch effects only occur when entity is close (<10m); leaves distant view crisp and visible
		{
			// Line of sight check to monster
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GazeTrace), false, this);
			QueryParams.AddIgnoredActor(CachedNearestEntity.Get());
			FHitResult HitResult;
			const bool bBlocked = World->LineTraceSingleByChannel(HitResult, CamLoc, MonsterHeadLoc, ECC_Visibility, QueryParams);

			if (!bBlocked)
			{
				const FVector DirToMonster = ToMonster.GetSafeNormal();
				const float LookAtDot = FVector::DotProduct(CamFwd, DirToMonster);

				// Dot > 0.45 means monster is inside the camera view cone
				if (LookAtDot > 0.45f)
				{
					// Direct gaze factor (0 at 0.45, 1.0 at 0.95+ looking right at the entity)
					const float CenterFactor = FMath::Clamp((LookAtDot - 0.45f) / 0.50f, 0.0f, 1.0f);
					// Distance factor: 1.0 at < 2.5m, fading out at 10m
					const float DistFactor = 1.0f - FMath::Clamp((Dist - 250.0f) / 750.0f, 0.0f, 1.0f);

					TargetGlitch = CenterFactor * DistFactor;
				}
			}
		}
	}

	// Smoothly interpolate current gaze glitch factor
	CurrentGazeGlitchFactor = FMath::FInterpTo(CurrentGazeGlitchFactor, TargetGlitch, DeltaTime, (TargetGlitch > CurrentGazeGlitchFactor) ? 4.0f : 2.5f);

	// Modulate FirstPersonCamera PostProcessSettings
	if (FirstPersonCamera)
	{
		FPostProcessSettings& PP = FirstPersonCamera->PostProcessSettings;

		if (CurrentGazeGlitchFactor > 0.01f)
		{
			// Chromatic aberration (SceneFringe) violently tears RGB channels
			PP.bOverride_SceneFringeIntensity = true;
			PP.SceneFringeIntensity = CurrentGazeGlitchFactor * 5.0f;

			// Creeping dark shadow vignette / tunnel vision
			PP.bOverride_VignetteIntensity = true;
			PP.VignetteIntensity = FMath::Clamp(0.35f + CurrentGazeGlitchFactor * 0.65f, 0.0f, 1.0f);

			// Grain / analog VHS noise
			PP.bOverride_FilmGrainIntensity = true;
			PP.FilmGrainIntensity = CurrentGazeGlitchFactor * 0.95f;

			// Desaturation to sickly cold horror tone
			PP.bOverride_ColorSaturation = true;
			const float SatVal = FMath::Lerp(1.0f, 0.45f, CurrentGazeGlitchFactor);
			PP.ColorSaturation = FVector4(SatVal, SatVal, SatVal, 1.0f);

			// Procedural dread camera tremor
			GazeTremorTime += DeltaTime * 35.0f;
			const float ShakeAmp = CurrentGazeGlitchFactor * 1.8f;
			const FVector TremorOffset(
				FMath::Sin(GazeTremorTime * 1.2f) * ShakeAmp * 0.5f,
				FMath::Cos(GazeTremorTime * 1.5f) * ShakeAmp,
				FMath::Sin(GazeTremorTime * 0.9f) * ShakeAmp
			);
			FirstPersonCamera->SetRelativeLocation(BaseHeadRelativeLocation + CurrentCameraOffset + TremorOffset);

			// Flashlight erratic stutter when aimed directly at monster
			if (FlashlightComponent && FlashlightComponent->IsVisible())
			{
				const float Flicker = FMath::RandRange(1.0f - CurrentGazeGlitchFactor * 0.5f, 1.0f);
				FlashlightComponent->SetIntensity(FlashlightBaseIntensity * Flicker);
			}
		}
		else
		{
			// Restore normal camera state
			PP.bOverride_SceneFringeIntensity = false;
			PP.SceneFringeIntensity = 0.0f;

			PP.bOverride_VignetteIntensity = true;
			PP.VignetteIntensity = 0.35f;

			PP.bOverride_FilmGrainIntensity = false;
			PP.FilmGrainIntensity = 0.0f;

			PP.bOverride_ColorSaturation = false;
			PP.ColorSaturation = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

			FirstPersonCamera->SetRelativeLocation(BaseHeadRelativeLocation + CurrentCameraOffset);

			if (FlashlightComponent && FlashlightComponent->IsVisible())
			{
				FlashlightComponent->SetIntensity(FlashlightBaseIntensity);
			}
		}
	}
}

void ABackroomsCharacter::TriggerJumpscareCaught(ABackroomsEntity* Attacker)
{
	if (bIsUnderAttack)
	{
		return;
	}

	bIsUnderAttack = true;

	// Disable player input during jumpscare
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		DisableInput(PC);
	}

	// Stop character locomotion
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}

	// Extreme visual trauma surge
	if (FirstPersonCamera)
	{
		FPostProcessSettings& PP = FirstPersonCamera->PostProcessSettings;
		PP.bOverride_SceneFringeIntensity = true;
		PP.SceneFringeIntensity = 5.0f;
		PP.bOverride_VignetteIntensity = true;
		PP.VignetteIntensity = 1.0f;
		PP.bOverride_ColorGain = true;
		PP.ColorGain = FVector4(1.6f, 0.3f, 0.3f, 1.0f); // Bloody red flash
	}

	// Camera Fade to black
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.45f, FColor::Black, true, true);
	}

	OnJumpscareCaught(Attacker);

	// Wakeup sequence after 2.0s blackout
	if (UWorld* World = GetWorld())
	{
		FTimerHandle WakeupTimerHandle;
		World->GetTimerManager().SetTimer(WakeupTimerHandle, [this, PC]()
		{
			// Respawn player at initial level start location
			SetActorLocation(RespawnLocation);
			if (PC)
			{
				PC->SetControlRotation(RespawnRotation);
			}

			// Clear damage post process
			if (FirstPersonCamera)
			{
				FPostProcessSettings& PP = FirstPersonCamera->PostProcessSettings;
				PP.bOverride_SceneFringeIntensity = false;
				PP.SceneFringeIntensity = 0.0f;
				PP.bOverride_VignetteIntensity = true;
				PP.VignetteIntensity = 0.35f;
				PP.bOverride_ColorGain = false;
				PP.ColorGain = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
				PP.bOverride_ColorSaturation = false;
				PP.ColorSaturation = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
				PP.bOverride_FilmGrainIntensity = false;
				PP.FilmGrainIntensity = 0.0f;
				FirstPersonCamera->SetRelativeLocation(BaseHeadRelativeLocation + CurrentCameraOffset);
			}

			// Fade back in from black
			if (PC && PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.4f, FColor::Black, false, false);
			}

			// Re-enable player input
			if (PC)
			{
				EnableInput(PC);
			}

			// Reset panic and state
			CurrentPanicFactor = 0.0f;
			CurrentGazeGlitchFactor = 0.0f;
			bIsUnderAttack = false;

			UE_LOG(LogTemp, Warning, TEXT("[ABackroomsCharacter] Player woke up at level spawn after jumpscare encounter."));
		}, 2.0f, false);
	}
}

