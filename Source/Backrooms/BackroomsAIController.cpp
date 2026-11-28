// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsAIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "BackroomsEntity.h"
#include "BackroomsCharacter.h"
#include "Components/CapsuleComponent.h"

ABackroomsAIController::ABackroomsAIController()
	: CurrentState(EBackroomsAIState::DistantStalker)
	, TargetPlayer(nullptr)
	, bIsChasing(false)
	, LostSightSearchDuration(5.0f)   // Realistic stalker search before resuming patrol
	, SearchLingerRadius(500.0f)       // Lingers tightly near the last seen location
	, WanderRadius(1200.0f)
	, WanderSpeed(180.0f)              // Quiet, deliberate stalker wander
	, SearchSpeed(240.0f)              // Deliberate, cautious stalking speed
	, ChaseSpeed(450.0f)               // Tense pursuit speed (player sprint is 600, walk is 300)
	, ChaseSound(nullptr)
	, StalkingMinDistance(1800.0f)     // Minimum distance for distant stalking sightings (18m)
	, StalkingMaxDistance(3400.0f)     // Maximum distance for distant stalking sightings (34m)
	, AppearanceIntervalMin(18.0f)     // Minimum appearance interval after encounter ends (18s)
	, AppearanceIntervalMax(35.0f)     // Maximum appearance interval after encounter ends (35s)
	, MinVisibleDuration(2.0f)         // Minimum visible time before allowed to vanish on look-away (2s)
	, MaxVisibleDuration(15.0f)        // Maximum visible duration if player never looks away (15s)
	, StalkerVanishLookAwayDelay(0.5f) // Time player must look away before entity vanishes
	, StalkerSpottedTimeThreshold(0.6f) // Direct gaze time before spotting is confirmed
	, FailedSpawnRetryDelayMin(2.0f)   // Retry delay min if no valid vantage found (2s)
	, FailedSpawnRetryDelayMax(4.0f)   // Retry delay max if no valid vantage found (4s)
	, StalkerUnseenRelocateTimeout(30.0f) // Timeout if player walks in opposite direction (30s)
	, StalkerCloseProximityThreshold(850.0f) // Vanishes if player sprints directly to it (8.5m)
	, bHasPlayerSpottedStalker(false)
	, bIsStalkerVanished(false)
	, CurrentEncounterVisibleTime(0.0f)
	, EvasionBreakTime(1.2f)           // Player sprint away duration to break chase
	, EvasionDistanceThreshold(700.0f) // Minimum distance to disengage
	, EvasionCooldownDuration(6.0f)    // Grace period before footsteps can re-aggro
	, MaxChaseDuration(12.0f)          // Maximum pursuit time before giving up
	, LastKnownPlayerLocation(FVector::ZeroVector)
	, PlayerEvasionTime(0.0f)
	, ChaseDurationTimer(0.0f)
	, EvasionReAggroCooldownTimer(0.0f)
	, StalkerLookDuration(0.0f)
	, StalkerLookAwayDuration(0.0f)
	, StalkerUnseenTime(0.0f)
{
	PrimaryActorTick.bCanEverTick = true;
	bStartAILogicOnPossess = true;
	bStopAILogicOnUnposses = true;

	// Create Perception Component
	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SetPerceptionComponent(*Perception);

	// Configure Sight Sense for "Stealthy Stalker" (focused forward vision cone)
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1400.0f;                    // ~14m
	SightConfig->LoseSightRadius = 1700.0f;                // ~17m
	SightConfig->PeripheralVisionAngleDegrees = 50.0f;     // 50° half-angle (100° total cone: player can sneak past behind/flank)
	SightConfig->SetMaxAge(5.0f);                          // 5s sight memory instead of indefinite pursuit
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	Perception->ConfigureSense(*SightConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());

	// Initialize Blackboard key selectors with default names and type filters
	AIStateKey.SelectedKeyName = FName(TEXT("AIState"));
	AIStateKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(ABackroomsAIController, AIStateKey), StaticEnum<EBackroomsAIState>());

	TargetActorKey.SelectedKeyName = FName(TEXT("TargetActor"));
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ABackroomsAIController, TargetActorKey), AActor::StaticClass());

	HasTargetKey.SelectedKeyName = FName(TEXT("HasTarget"));
	HasTargetKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(ABackroomsAIController, HasTargetKey));

	TargetLocationKey.SelectedKeyName = FName(TEXT("TargetLocation"));
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ABackroomsAIController, TargetLocationKey));
}

void ABackroomsAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
	else if (BlackboardAsset)
	{
		UBlackboardComponent* BB = nullptr;
		UseBlackboard(BlackboardAsset, BB);
	}

	// Resolve key selectors against the active BlackboardData
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		if (const UBlackboardData* BBData = BB->GetBlackboardAsset())
		{
			AIStateKey.ResolveSelectedKey(*BBData);
			TargetActorKey.ResolveSelectedKey(*BBData);
			HasTargetKey.ResolveSelectedKey(*BBData);
			TargetLocationKey.ResolveSelectedKey(*BBData);
		}
	}

	if (UAIPerceptionComponent* Perception = GetAIPerceptionComponent())
	{
		Perception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ABackroomsAIController::OnTargetPerceptionUpdated);
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ABackroomsAIController::OnTargetPerceptionUpdated);
	}

	CurrentState = EBackroomsAIState::DistantStalker;
	SetEntityMovementSpeed(0.0f);
	UpdateBlackboardKeys();

	// Schedule initial appearance after 1.5s so player pawn is fully spawned and camera is initialized
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StalkerAppearanceTimerHandle);
		World->GetTimerManager().SetTimer(StalkerAppearanceTimerHandle, this, &ABackroomsAIController::AttemptAppearance, 1.5f, false);
	}
}

void ABackroomsAIController::OnUnPossess()
{
	if (UAIPerceptionComponent* Perception = GetAIPerceptionComponent())
	{
		Perception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ABackroomsAIController::OnTargetPerceptionUpdated);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LostSightTimerHandle);
		World->GetTimerManager().ClearTimer(WanderWaitTimerHandle);
		World->GetTimerManager().ClearTimer(ChaseNextTickTimerHandle);
		World->GetTimerManager().ClearTimer(StalkerAppearanceTimerHandle);
	}

	TargetPlayer = nullptr;
	bIsChasing = false;
	StopMovement();

	Super::OnUnPossess();
}

void ABackroomsAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LostSightTimerHandle);
		World->GetTimerManager().ClearTimer(WanderWaitTimerHandle);
		World->GetTimerManager().ClearTimer(ChaseNextTickTimerHandle);
		World->GetTimerManager().ClearTimer(StalkerAppearanceTimerHandle);
	}

	TargetPlayer = nullptr;
	bIsChasing = false;

	Super::EndPlay(EndPlayReason);
}

void ABackroomsAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if !UE_BUILD_SHIPPING
	if (APawn* ControlledPawn = GetPawn())
	{
		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			// Read AIState directly from the Blackboard component using type-safe selector
			const FName StateKey = AIStateKey.SelectedKeyName.IsNone() ? FName(TEXT("AIState")) : AIStateKey.SelectedKeyName;
			uint8 StateVal = BB->GetValueAsEnum(StateKey);
			EBackroomsAIState StateFromBB = static_cast<EBackroomsAIState>(StateVal);
			FString StateStr;
			FColor StateColor = FColor::Green;

			switch (StateFromBB)
			{
			case EBackroomsAIState::DistantStalker:
				StateStr = bIsStalkerVanished ? TEXT("DISTANT STALKER (Vanished)") : (bHasPlayerSpottedStalker ? TEXT("DISTANT STALKER (SPOTTED - Watching)") : TEXT("DISTANT STALKER (Watching)"));
				StateColor = FColor::Purple;
				break;
			case EBackroomsAIState::IdleWander:
				StateStr = TEXT("IDLE WANDER (Silent)");
				StateColor = FColor::Green;
				break;
			case EBackroomsAIState::Chase:
				StateStr = TEXT("CHASE (Loud)");
				StateColor = FColor::Red;
				break;
			case EBackroomsAIState::Search:
				StateStr = FString::Printf(TEXT("SEARCHING (%.1fs left)"), (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(LostSightTimerHandle)) ? GetWorld()->GetTimerManager().GetTimerRemaining(LostSightTimerHandle) : 0.0f);
				StateColor = FColor::Yellow;
				break;
			}

			if (GetWorld() && !ControlledPawn->IsActorBeingDestroyed())
			{
				FVector TextLocation = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 110.0f);
				DrawDebugString(GetWorld(), TextLocation, FString::Printf(TEXT("[BB_AIState: %s]"), *StateStr), nullptr, StateColor, 0.0f, false, 1.2f);
			}
		}
	}
#endif

	// Cooldown timer update
	EvasionReAggroCooldownTimer = FMath::Max(0.0f, EvasionReAggroCooldownTimer - DeltaTime);

	// Primary Stalking Loop
	if (CurrentState == EBackroomsAIState::DistantStalker)
	{
		UpdateDistantStalker(DeltaTime);
	}
	// Evaluate player evasion and chase endurance during active pursuit
	else if (CurrentState == EBackroomsAIState::Chase)
	{
		UpdateEvasionCheck(DeltaTime);

		// Check attack proximity to player
		if (APawn* MyPawn = GetPawn())
		{
			if (ABackroomsCharacter* PlayerChar = Cast<ABackroomsCharacter>(TargetPlayer.Get()))
			{
				if (!PlayerChar->IsActorBeingDestroyed())
				{
					const float Dist = FVector::Dist(MyPawn->GetActorLocation(), PlayerChar->GetActorLocation());
					ABackroomsEntity* MyEntity = Cast<ABackroomsEntity>(MyPawn);
					const float AttackDist = MyEntity ? MyEntity->AttackRange : 140.0f;

					if (Dist <= AttackDist && MyEntity && !MyEntity->bIsAttacking)
					{
						StopMovement();
						ClearFocus(EAIFocusPriority::Gameplay);
						MyEntity->ExecuteAttack(PlayerChar);
						SmartDisengage(TEXT("Monster executed jumpscare attack"));
					}
				}
			}
		}
	}

	// Audio-based detection: hear sprinting player within 650cm when not on evasion cooldown (only in wander/search)
	if (CurrentState != EBackroomsAIState::Chase && CurrentState != EBackroomsAIState::DistantStalker && EvasionReAggroCooldownTimer <= 0.0f && GetPawn() && !GetPawn()->IsActorBeingDestroyed())
	{
		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (ACharacter* PlayerChar = Cast<ACharacter>(PC->GetPawn()))
				{
					if (!PlayerChar->IsActorBeingDestroyed())
					{
						const float DistSq = FVector::DistSquared(GetPawn()->GetActorLocation(), PlayerChar->GetActorLocation());
						const float HearingRadius = 650.0f;
						if (DistSq <= FMath::Square(HearingRadius))
						{
							// Player walking is 300, sprint is 600 - trigger only on sprint
							if (PlayerChar->GetVelocity().Size2D() > 450.0f)
							{
								CurrentState = EBackroomsAIState::Chase;
								TargetPlayer = PlayerChar;
								bIsChasing = true;
								PlayerEvasionTime = 0.0f;
								ChaseDurationTimer = 0.0f;
								LastKnownPlayerLocation = PlayerChar->GetActorLocation();

								SetEntityMovementSpeed(ChaseSpeed);
								SetFocus(PlayerChar);
								MoveToActor(PlayerChar, 80.0f);
								UpdateBlackboardKeys();
								UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Heard sprinting footsteps within 650cm! Entering Chase."));
							}
						}
					}
				}
			}
		}
	}
}

void ABackroomsAIController::UpdateEvasionCheck(float DeltaTime)
{
	if (CurrentState != EBackroomsAIState::Chase)
	{
		PlayerEvasionTime = 0.0f;
		ChaseDurationTimer = 0.0f;
		return;
	}

	APawn* MyPawn = GetPawn();
	AActor* Target = TargetPlayer.Get();
	ACharacter* PlayerChar = Cast<ACharacter>(Target);

	if (!MyPawn || !PlayerChar || MyPawn->IsActorBeingDestroyed() || PlayerChar->IsActorBeingDestroyed())
	{
		SmartDisengage(TEXT("Target or Pawn became invalid"));
		return;
	}

	const FVector EntityLoc = MyPawn->GetActorLocation();
	const FVector PlayerLoc = PlayerChar->GetActorLocation();
	const FVector PlayerVel = PlayerChar->GetVelocity();
	const float PlayerSpeed = PlayerVel.Size2D();
	const float Distance = FVector::Dist(EntityLoc, PlayerLoc);

	// Unit direction from monster towards player
	const FVector ToPlayerDir = (PlayerLoc - EntityLoc).GetSafeNormal2D();
	const FVector PlayerMoveDir = PlayerVel.GetSafeNormal2D();

	// Dot product: > 0 means player is moving away from monster, ~ 1.0 means sprinting directly away
	const float RetreatDot = FVector::DotProduct(ToPlayerDir, PlayerMoveDir);

	// User requirement: When player runs fast in opposite direction, entity smartly stops chasing
	// Sprint speed is 600 cm/s; threshold > 400 cm/s moving away (RetreatDot > 0.45)
	const bool bIsRunningAway = (PlayerSpeed > 400.0f) && (RetreatDot > 0.45f);

	if (bIsRunningAway)
	{
		PlayerEvasionTime += DeltaTime;

		// Disengage if player has maintained an escape sprint for EvasionBreakTime and is beyond arm's reach (>EvasionDistanceThreshold)
		// OR if player has managed to put significant distance between them (> 1300cm)
		if ((PlayerEvasionTime >= EvasionBreakTime && Distance >= EvasionDistanceThreshold) || Distance > 1300.0f)
		{
			SmartDisengage(TEXT("Player sprinted away in opposite direction"));
			return;
		}
	}
	else
	{
		// Player slowed down or turned towards monster -> reduce accumulated evasion credit
		PlayerEvasionTime = FMath::Max(0.0f, PlayerEvasionTime - DeltaTime * 2.0f);
	}

	// Maximum pursuit endurance: monster gives up after chasing continuously without success
	ChaseDurationTimer += DeltaTime;
	if (ChaseDurationTimer >= MaxChaseDuration && Distance > 500.0f)
	{
		SmartDisengage(TEXT("Monster fatigued after long pursuit"));
		return;
	}
}

void ABackroomsAIController::SmartDisengage(const FString& Reason)
{
	UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] SmartDisengage: %s. Ceasing pursuit."), *Reason);

	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	bIsChasing = false;
	TargetPlayer = nullptr;
	PlayerEvasionTime = 0.0f;
	ChaseDurationTimer = 0.0f;

	// Cooldown prevents immediate re-aggro from footsteps while player is escaping
	EvasionReAggroCooldownTimer = EvasionCooldownDuration;

	// Transition to brief search / pause state
	CurrentState = EBackroomsAIState::Search;
	SetEntityMovementSpeed(SearchSpeed);
	UpdateBlackboardKeys();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LostSightTimerHandle);
		World->GetTimerManager().ClearTimer(ChaseNextTickTimerHandle);

		// Search for 4 seconds, then smoothly transition back to quiet IdleWander
		World->GetTimerManager().SetTimer(LostSightTimerHandle, this, &ABackroomsAIController::OnLoseSightTimerExpired, 4.0f, false);
	}
}

void ABackroomsAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	// If the move was aborted (e.g. from unpossess, cleanup, or new request), do not schedule new moves
	if (Result.Code == EPathFollowingResult::Aborted || IsActorBeingDestroyed() || !GetPawn() || GetPawn()->IsActorBeingDestroyed())
	{
		return;
	}

	if (CurrentState == EBackroomsAIState::Chase)
	{
		AActor* Target = TargetPlayer.Get();
		if (IsValid(Target) && !Target->IsActorBeingDestroyed() && GetPawn() && !GetPawn()->IsActorBeingDestroyed())
		{
			// Check if already in attack range
			ABackroomsEntity* MyEntity = Cast<ABackroomsEntity>(GetPawn());
			ABackroomsCharacter* PlayerChar = Cast<ABackroomsCharacter>(Target);
			const float Dist = FVector::Dist(GetPawn()->GetActorLocation(), Target->GetActorLocation());
			const float AttackDist = MyEntity ? MyEntity->AttackRange : 140.0f;

			if (Dist <= AttackDist && MyEntity && PlayerChar && !MyEntity->bIsAttacking)
			{
				StopMovement();
				ClearFocus(EAIFocusPriority::Gameplay);
				MyEntity->ExecuteAttack(PlayerChar);
				SmartDisengage(TEXT("Monster reached player and executed attack"));
			}
			else
			{
				// Defer to next tick to prevent re-entrant synchronous recursion in OnMoveCompleted
				if (UWorld* World = GetWorld())
				{
					World->GetTimerManager().ClearTimer(ChaseNextTickTimerHandle);
					World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
					{
						if (CurrentState == EBackroomsAIState::Chase && GetPawn() && !GetPawn()->IsActorBeingDestroyed())
						{
							AActor* GoalActor = TargetPlayer.Get();
							if (IsValid(GoalActor) && !GoalActor->IsActorBeingDestroyed())
							{
								MoveToActor(GoalActor, 80.0f);
							}
						}
					}));
				}
			}
		}
	}
	else if (CurrentState == EBackroomsAIState::Search)
	{
		// Lingering search: wander specifically near last known player location
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(WanderWaitTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (CurrentState == EBackroomsAIState::Search && GetPawn() && !GetPawn()->IsActorBeingDestroyed())
				{
					MoveToReachablePointNear(LastKnownPlayerLocation, SearchLingerRadius);
				}
			}), FMath::RandRange(1.0f, 2.5f), false);
		}
	}
	else if (CurrentState == EBackroomsAIState::IdleWander)
	{
		// Passive corridor wander
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(WanderWaitTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (CurrentState == EBackroomsAIState::IdleWander && GetPawn() && !GetPawn()->IsActorBeingDestroyed())
				{
					MoveToRandomReachablePoint(WanderRadius);
				}
			}), FMath::RandRange(2.0f, 4.0f), false);
		}
	}
}

void ABackroomsAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || Actor == GetPawn() || IsActorBeingDestroyed() || !GetPawn() || GetPawn()->IsActorBeingDestroyed())
	{
		return;
	}

	ACharacter* PlayerChar = Cast<ACharacter>(Actor);
	if (!PlayerChar || !PlayerChar->IsPlayerControlled() || PlayerChar->IsActorBeingDestroyed())
	{
		return;
	}

	// Distant Stalker maintains silent, eerie stillness down hallways - never immediately charges on sight
	if (CurrentState == EBackroomsAIState::DistantStalker)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// SIGHT ACQUIRED -> Transition to Chase
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(LostSightTimerHandle);
			World->GetTimerManager().ClearTimer(WanderWaitTimerHandle);
			World->GetTimerManager().ClearTimer(ChaseNextTickTimerHandle);
		}

		// Do not play audio cue here: audio is strictly sight-gated on player camera to prevent spoilers from behind
		CurrentState = EBackroomsAIState::Chase;
		TargetPlayer = Actor;
		bIsChasing = true;
		PlayerEvasionTime = 0.0f;
		ChaseDurationTimer = 0.0f;
		LastKnownPlayerLocation = Stimulus.StimulusLocation;

		SetEntityMovementSpeed(ChaseSpeed);
		SetFocus(Actor);
		MoveToActor(Actor, 80.0f);

		UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Player detected in forward 45 deg cone! Entering Chase."));

		UpdateBlackboardKeys();
	}
	else
	{
		// SIGHT LOST -> Transition to Search with velocity prediction
		ClearFocus(EAIFocusPriority::Gameplay);
		bIsChasing = false;
		CurrentState = EBackroomsAIState::Search;

		// Velocity prediction: predict player position along their movement vector for 2.0s
		const FVector PlayerVel = PlayerChar ? PlayerChar->GetVelocity() : FVector::ZeroVector;
		LastKnownPlayerLocation = Stimulus.StimulusLocation + (PlayerVel * 2.0f);

		SetEntityMovementSpeed(SearchSpeed);

		// Move directly toward predicted location
		MoveToLocation(LastKnownPlayerLocation, 60.0f);

		UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Player lost sight! Searching near predicted location for %.1f s."), LostSightSearchDuration);

		UpdateBlackboardKeys();

		// Start long memory search timer (15-20s)
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(LostSightTimerHandle, this, &ABackroomsAIController::OnLoseSightTimerExpired, LostSightSearchDuration, false);
		}
	}
}

void ABackroomsAIController::OnLoseSightTimerExpired()
{
	// Search timer expired without re-acquiring player -> Transition back to Distant Stalker
	TargetPlayer = nullptr;
	bIsChasing = false;
	CurrentState = EBackroomsAIState::DistantStalker;
	ClearFocus(EAIFocusPriority::Gameplay);

	SetEntityMovementSpeed(0.0f);
	UpdateBlackboardKeys();

	UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Search timer expired. Relocating to distant stalker vantage."));

	VanishFromSight(TEXT("Search expired - returning to shadows"));
}

void ABackroomsAIController::MoveToRandomReachablePoint(float Radius)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || ControlledPawn->IsActorBeingDestroyed())
	{
		return;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		return;
	}

	FNavLocation RandomLocation;
	if (NavSys->GetRandomReachablePointInRadius(ControlledPawn->GetActorLocation(), Radius, RandomLocation))
	{
		MoveToLocation(RandomLocation.Location, 50.0f);

		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			const FName LocationKey = TargetLocationKey.SelectedKeyName.IsNone() ? FName(TEXT("TargetLocation")) : TargetLocationKey.SelectedKeyName;
			BB->SetValueAsVector(LocationKey, RandomLocation.Location);
		}
	}
}

void ABackroomsAIController::MoveToReachablePointNear(const FVector& Center, float Radius)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || ControlledPawn->IsActorBeingDestroyed())
	{
		return;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		return;
	}

	FNavLocation RandomLocation;
	if (NavSys->GetRandomReachablePointInRadius(Center, Radius, RandomLocation))
	{
		MoveToLocation(RandomLocation.Location, 50.0f);

		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			const FName LocationKey = TargetLocationKey.SelectedKeyName.IsNone() ? FName(TEXT("TargetLocation")) : TargetLocationKey.SelectedKeyName;
			BB->SetValueAsVector(LocationKey, RandomLocation.Location);
		}
	}
}

void ABackroomsAIController::SetEntityMovementSpeed(float Speed)
{
	if (ACharacter* Char = Cast<ACharacter>(GetPawn()))
	{
		if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = Speed;
		}
	}
}

void ABackroomsAIController::UpdateBlackboardKeys()
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		const FName StateKey = AIStateKey.SelectedKeyName.IsNone() ? FName(TEXT("AIState")) : AIStateKey.SelectedKeyName;
		const FName TargetKey = TargetActorKey.SelectedKeyName.IsNone() ? FName(TEXT("TargetActor")) : TargetActorKey.SelectedKeyName;
		const FName HasTargetK = HasTargetKey.SelectedKeyName.IsNone() ? FName(TEXT("HasTarget")) : HasTargetKey.SelectedKeyName;
		const FName LocationKey = TargetLocationKey.SelectedKeyName.IsNone() ? FName(TEXT("TargetLocation")) : TargetLocationKey.SelectedKeyName;

		BB->SetValueAsEnum(StateKey, static_cast<uint8>(CurrentState));

		AActor* Target = TargetPlayer.Get();
		if (IsValid(Target) && !Target->IsActorBeingDestroyed())
		{
			BB->SetValueAsObject(TargetKey, Target);
			BB->SetValueAsBool(HasTargetK, true);
		}
		else
		{
			BB->ClearValue(TargetKey);
			BB->SetValueAsBool(HasTargetK, false);
		}

		BB->SetValueAsVector(LocationKey, LastKnownPlayerLocation);
	}
}

bool ABackroomsAIController::ValidateAndAlignGroundLocation(const FVector& InNavLocation, FVector& OutSpawnLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	APawn* MyPawn = GetPawn();
	const ACharacter* MyChar = Cast<ACharacter>(MyPawn);
	const UCapsuleComponent* Capsule = MyChar ? MyChar->GetCapsuleComponent() : nullptr;

	const float CapsuleHalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 96.0f;
	const float CapsuleRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : 42.0f;

	// 1. Downward ground trace to find actual physical floor surface
	// NavMesh vertices can vary vertically from physical floor meshes by tens of centimeters
	const FVector TraceStart = FVector(InNavLocation.X, InNavLocation.Y, InNavLocation.Z + 200.0f);
	const FVector TraceEnd = FVector(InNavLocation.X, InNavLocation.Y, InNavLocation.Z - 350.0f);

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(EntityGroundTrace), false, MyPawn);
	FHitResult FloorHit;

	// Trace against WorldStatic first (floors/terrain/architecture)
	bool bHit = World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams);
	if (!bHit)
	{
		// Fallback to Visibility channel
		bHit = World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Visibility, TraceParams);
	}

	if (!bHit)
	{
		return false;
	}

	// Surface normal check: ensure the hit surface is a walkable floor (not a vertical wall or steep slope)
	if (FloorHit.Normal.Z < 0.65f)
	{
		return false;
	}

	// 2. Compute exact capsule center height so the capsule bottom rests precisely on the floor surface
	// with a 1.5cm safety margin to prevent any initial penetration
	const float GroundBuffer = 1.5f;
	const float CapsuleCenterZ = FloorHit.ImpactPoint.Z + CapsuleHalfHeight + GroundBuffer;
	const FVector CandidateCenter = FVector(InNavLocation.X, InNavLocation.Y, CapsuleCenterZ);

	// 3. Collision overlap check: verify capsule does not intersect nearby walls, props, or obstacles
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius * 0.92f, CapsuleHalfHeight * 0.92f);
	FCollisionQueryParams OverlapParams(SCENE_QUERY_STAT(EntityCapsuleOverlap), false, MyPawn);

	const bool bOverlapping = World->OverlapBlockingTestByChannel(
		CandidateCenter,
		FQuat::Identity,
		ECC_WorldStatic,
		CapsuleShape,
		OverlapParams
	);

	if (bOverlapping)
	{
		return false;
	}

	// 4. Vertical clearance trace upward to ensure sufficient headroom
	FHitResult HeadHit;
	const FVector HeadTraceStart = CandidateCenter;
	const FVector HeadTraceEnd = CandidateCenter + FVector(0.0f, 0.0f, CapsuleHalfHeight + 25.0f);
	if (World->LineTraceSingleByChannel(HeadHit, HeadTraceStart, HeadTraceEnd, ECC_WorldStatic, TraceParams))
	{
		return false; // Insufficient ceiling clearance
	}

	OutSpawnLocation = CandidateCenter;
	return true;
}

bool ABackroomsAIController::FindDistantStalkingLocation(const AActor* InPlayerActor, FVector& OutLocation, FRotator& OutRotation)
{
	if (!InPlayerActor || !GetWorld())
	{
		return false;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		return false;
	}

	const FVector PlayerLoc = InPlayerActor->GetActorLocation();
	FVector CamLoc = PlayerLoc + FVector(0.0f, 0.0f, 60.0f);
	FVector CamForward = InPlayerActor->GetActorForwardVector();

	if (const ACharacter* Char = Cast<ACharacter>(InPlayerActor))
	{
		if (const APlayerController* PC = Cast<APlayerController>(Char->GetController()))
		{
			if (PC->PlayerCameraManager)
			{
				CamLoc = PC->PlayerCameraManager->GetCameraLocation();
				CamForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
			}
		}
	}

	CamForward.Z = 0.0f;
	CamForward.Normalize();

	struct FStalkCandidate
	{
		FVector Location;
		float Score;
	};

	TArray<FStalkCandidate> Candidates;

	// Sweep candidate angles relative to player camera forward:
	// Direct corridor angles first (±0°, ±8°, ±16°), then branching intersection angles (±25°, ±40°, ±60°, ±85°), then flank angles
	const float TestAngles[] = {
		0.0f, 8.0f, -8.0f, 16.0f, -16.0f, 25.0f, -25.0f,
		40.0f, -40.0f, 60.0f, -60.0f, 85.0f, -85.0f, 110.0f, -110.0f,
		140.0f, -140.0f, 180.0f
	};

	// Sample corridor distances between 1900cm (19m) and 3400cm (34m)
	const float TestDistances[] = { 1900.0f, 2300.0f, 2700.0f, 3100.0f, 3400.0f };

	for (float Angle : TestAngles)
	{
		const FRotator Rot(0.0f, Angle, 0.0f);
		const FVector Dir = Rot.RotateVector(CamForward);

		for (float Dist : TestDistances)
		{
			const FVector QueryPoint = PlayerLoc + (Dir * Dist);

			FNavLocation NavLoc;
			if (NavSys->ProjectPointToNavigation(QueryPoint, NavLoc, FVector(250.0f, 250.0f, 300.0f)))
			{
				const float ActualDist = FVector::Dist(PlayerLoc, NavLoc.Location);
				if (ActualDist < StalkingMinDistance || ActualDist > StalkingMaxDistance)
				{
					continue;
				}

				// Perform precision physics ground trace and collision clearance validation
				FVector AlignedSpawnLoc;
				if (!ValidateAndAlignGroundLocation(NavLoc.Location, AlignedSpawnLoc))
				{
					continue; // Rejected due to floor penetration, wall collision, or low ceiling
				}

				// Line of sight trace from player camera to candidate monster chest
				const FVector TargetChest = AlignedSpawnLoc + FVector(0.0f, 0.0f, 30.0f);
				FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(StalkerLOSTrace), false, InPlayerActor);
				if (GetPawn())
				{
					TraceParams.AddIgnoredActor(GetPawn());
				}

				FHitResult HitResult;
				const bool bBlocked = GetWorld()->LineTraceSingleByChannel(HitResult, CamLoc, TargetChest, ECC_Visibility, TraceParams);

				const FVector ToNavDir = (AlignedSpawnLoc - PlayerLoc).GetSafeNormal2D();
				const float Dot = FVector::DotProduct(CamForward, ToNavDir);

				if (!bBlocked)
				{
					// Direct corridor visibility in player's perspective
					float Score = 100.0f - (FMath::Abs(ActualDist - 2500.0f) * 0.015f);
					if (Dot > 0.25f)
					{
						// Strong priority for locations directly in front of the player's view down long hallways
						Score += 60.0f + (Dot * 30.0f);
					}

					Candidates.Add({ AlignedSpawnLoc, Score });
				}
				else
				{
					// Around a distant corner / intersection
					float Score = 35.0f - (FMath::Abs(ActualDist - 2500.0f) * 0.02f);
					Candidates.Add({ AlignedSpawnLoc, Score });
				}
			}
		}
	}

	if (Candidates.Num() == 0)
	{
		// Fallback: sample random reachable nav point in radius and validate ground clearance
		for (int32 Attempt = 0; Attempt < 8; ++Attempt)
		{
			FNavLocation FallbackNavLoc;
			const float TargetDist = FMath::RandRange(StalkingMinDistance, StalkingMaxDistance);
			if (NavSys->GetRandomReachablePointInRadius(PlayerLoc, TargetDist, FallbackNavLoc))
			{
				const float ActualDist = FVector::Dist(PlayerLoc, FallbackNavLoc.Location);
				if (ActualDist >= StalkingMinDistance * 0.85f && ActualDist <= StalkingMaxDistance * 1.15f)
				{
					FVector FallbackAligned;
					if (ValidateAndAlignGroundLocation(FallbackNavLoc.Location, FallbackAligned))
					{
						OutLocation = FallbackAligned;
						OutRotation = (PlayerLoc - OutLocation).Rotation();
						OutRotation.Pitch = 0.0f;
						OutRotation.Roll = 0.0f;
						return true;
					}
				}
			}
		}
		return false;
	}

	// Sort candidates by score descending
	Candidates.Sort([](const FStalkCandidate& A, const FStalkCandidate& B) {
		return A.Score > B.Score;
	});

	// Pick randomly from top 3 candidates to vary the vantage points
	const int32 NumTop = FMath::Min(3, Candidates.Num());
	const int32 SelectedIdx = FMath::RandRange(0, NumTop - 1);

	OutLocation = Candidates[SelectedIdx].Location;
	OutRotation = (PlayerLoc - OutLocation).Rotation();
	OutRotation.Pitch = 0.0f;
	OutRotation.Roll = 0.0f;

	return true;
}

bool ABackroomsAIController::RelocateToDistantStalkingVantage()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn || MyPawn->IsActorBeingDestroyed())
	{
		return false;
	}

	AActor* PlayerActor = TargetPlayer.Get();
	if (!PlayerActor)
	{
		PlayerActor = UGameplayStatics::GetPlayerCharacter(this, 0);
		TargetPlayer = PlayerActor;
	}

	if (!PlayerActor)
	{
		return false;
	}

	FVector VantageLoc;
	FRotator VantageRot;
	if (FindDistantStalkingLocation(PlayerActor, VantageLoc, VantageRot))
	{
		MyPawn->TeleportTo(VantageLoc, VantageRot);
		MyPawn->SetActorRotation(VantageRot);

		if (ACharacter* Char = Cast<ACharacter>(MyPawn))
		{
			const float CapsuleHalfHeight = Char->GetCapsuleComponent() ? Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.0f;
			if (USkeletalMeshComponent* MeshComp = Char->GetMesh())
			{
				MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -CapsuleHalfHeight));
				MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
			}

			if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
			{
				MoveComp->StopMovementImmediately();
				MoveComp->Velocity = FVector::ZeroVector;
				MoveComp->SetMovementMode(MOVE_Walking);

				FFindFloorResult FloorResult;
				MoveComp->FindFloor(MoveComp->UpdatedComponent->GetComponentLocation(), FloorResult, false);
				if (FloorResult.IsWalkableFloor())
				{
					MoveComp->AdjustFloorHeight();
				}
			}
		}

		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);

		bHasPlayerSpottedStalker = false;
		bIsStalkerVanished = false;
		StalkerLookDuration = 0.0f;
		StalkerLookAwayDuration = 0.0f;
		StalkerUnseenTime = 0.0f;
		CurrentEncounterVisibleTime = 0.0f;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(StalkerAppearanceTimerHandle);
		}

		if (ABackroomsEntity* Ent = Cast<ABackroomsEntity>(MyPawn))
		{
			Ent->SetStalkerVisibility(true);
		}

		if (ABackroomsCharacter* PlayerChar = Cast<ABackroomsCharacter>(PlayerActor))
		{
			PlayerChar->ResetSightAudioTrigger();
		}

		UpdateBlackboardKeys();

		UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Entity relocated to distant stalking vantage at %s (dist: %.0f cm). Motionless and silent."),
			*VantageLoc.ToString(), FVector::Dist(PlayerActor->GetActorLocation(), VantageLoc));

		return true;
	}

	return false;
}

void ABackroomsAIController::AttemptAppearance()
{
	if (CurrentState != EBackroomsAIState::DistantStalker)
	{
		return;
	}

	if (RelocateToDistantStalkingVantage())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Distant Stalker encounter initiated successfully."));
	}
	else
	{
		// No valid vantage point found from player's current perspective (e.g. player in tight blind corner).
		// Retry after a short delay (2 to 4 seconds) to ensure the encounter is never abandoned!
		const float RetryDelay = FMath::RandRange(FailedSpawnRetryDelayMin, FailedSpawnRetryDelayMax);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(StalkerAppearanceTimerHandle);
			World->GetTimerManager().SetTimer(StalkerAppearanceTimerHandle, this, &ABackroomsAIController::AttemptAppearance, RetryDelay, false);
		}
		UE_LOG(LogTemp, Log, TEXT("[BackroomsAI] No valid stalking vantage found; retrying in %.1f seconds."), RetryDelay);
	}
}

void ABackroomsAIController::VanishFromSight(const FString& Reason)
{
	UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] VanishFromSight: %s. Entity vanished into darkness."), *Reason);

	APawn* MyPawn = GetPawn();
	if (!MyPawn || MyPawn->IsActorBeingDestroyed())
	{
		return;
	}

	bIsStalkerVanished = true;
	bHasPlayerSpottedStalker = false;
	StalkerLookDuration = 0.0f;
	StalkerLookAwayDuration = 0.0f;
	StalkerUnseenTime = 0.0f;
	CurrentEncounterVisibleTime = 0.0f;

	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);

	if (ABackroomsEntity* Ent = Cast<ABackroomsEntity>(MyPawn))
	{
		Ent->SetStalkerVisibility(false);
	}

	// Move to holding location below level while vanished so it doesn't block player or navigation
	MyPawn->SetActorLocation(FVector(0.0f, 0.0f, -5000.0f));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StalkerAppearanceTimerHandle);
		// Establish reliable encounter timing: 18 to 35 seconds interval after previous encounter ends
		const float Cooldown = FMath::RandRange(AppearanceIntervalMin, AppearanceIntervalMax);
		World->GetTimerManager().SetTimer(StalkerAppearanceTimerHandle, this, &ABackroomsAIController::AttemptAppearance, Cooldown, false);
		UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Scheduled next distant stalker sighting in %.1f seconds."), Cooldown);
	}
}

void ABackroomsAIController::UpdateDistantStalker(float DeltaTime)
{
	if (bIsStalkerVanished)
	{
		return;
	}

	APawn* MyPawn = GetPawn();
	if (!MyPawn || MyPawn->IsActorBeingDestroyed())
	{
		return;
	}

	AActor* PlayerActor = TargetPlayer.Get();
	if (!PlayerActor)
	{
		PlayerActor = UGameplayStatics::GetPlayerCharacter(this, 0);
		TargetPlayer = PlayerActor;
	}

	if (!PlayerActor || PlayerActor->IsActorBeingDestroyed())
	{
		return;
	}

	// Accumulate visible duration for current encounter
	CurrentEncounterVisibleTime += DeltaTime;

	// Maximum visible duration (15s): If player stands staring at entity without looking away, dissolve into darkness
	if (CurrentEncounterVisibleTime >= MaxVisibleDuration)
	{
		VanishFromSight(TEXT("Exceeded maximum visible duration (15s timeout)"));
		return;
	}

	const FVector EntityLoc = MyPawn->GetActorLocation();
	const FVector PlayerLoc = PlayerActor->GetActorLocation();
	const float DistToPlayer = FVector::Dist(EntityLoc, PlayerLoc);

	// Close proximity safeguard: If player approaches within 8.5m, vanish into darkness before they close distance
	if (DistToPlayer <= StalkerCloseProximityThreshold)
	{
		VanishFromSight(TEXT("Player approached close to stalker position (<8.5m)"));
		return;
	}

	// Ensure entity stays still and continuously faces the player
	FRotator FaceRot = (PlayerLoc - EntityLoc).Rotation();
	FaceRot.Pitch = 0.0f;
	FaceRot.Roll = 0.0f;
	MyPawn->SetActorRotation(FaceRot);

	// Get player camera location and forward vector
	FVector CamLoc = PlayerLoc + FVector(0.0f, 0.0f, 60.0f);
	FVector CamForward = PlayerActor->GetActorForwardVector();

	if (const ACharacter* Char = Cast<ACharacter>(PlayerActor))
	{
		if (const APlayerController* PC = Cast<APlayerController>(Char->GetController()))
		{
			if (PC->PlayerCameraManager)
			{
				CamLoc = PC->PlayerCameraManager->GetCameraLocation();
				CamForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
			}
		}
	}

	const FVector TargetChest = EntityLoc + FVector(0.0f, 0.0f, 60.0f);
	const FVector ToMonster = (TargetChest - CamLoc);
	const float ToMonsterDist = ToMonster.Size();
	const FVector ToMonsterDir = ToMonster / FMath::Max(ToMonsterDist, 1.0f);

	const float LookDot = FVector::DotProduct(CamForward, ToMonsterDir);

	// Line of sight raycast
	bool bDirectSight = false;
	if (UWorld* World = GetWorld())
	{
		FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PlayerStalkerLOS), false, PlayerActor);
		TraceParams.AddIgnoredActor(MyPawn);
		FHitResult HitResult;
		const bool bBlocked = World->LineTraceSingleByChannel(HitResult, CamLoc, TargetChest, ECC_Visibility, TraceParams);
		bDirectSight = !bBlocked;
	}

	const bool bPlayerLookingDirectly = (LookDot > 0.40f) && bDirectSight;

	if (bPlayerLookingDirectly)
	{
		// Reset look-away timer immediately when player looks back at entity
		StalkerLookAwayDuration = 0.0f;

		StalkerLookDuration += DeltaTime;
		if (StalkerLookDuration >= StalkerSpottedTimeThreshold)
		{
			if (!bHasPlayerSpottedStalker)
			{
				bHasPlayerSpottedStalker = true;
				UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Player has spotted the distant stalker! Watching..."));
			}
		}
	}
	else
	{
		StalkerLookDuration = FMath::Max(0.0f, StalkerLookDuration - DeltaTime);
	}

	// Psychological horror vanish: Once player has spotted entity, if they look away or turn a corner -> VANISH!
	// Enforce minimum visible duration (~2.0s) so encounter is never cut short prematurely before player notices
	if (bHasPlayerSpottedStalker)
	{
		const bool bLookedAway = (!bDirectSight) || (LookDot < 0.28f);

		if (bLookedAway)
		{
			StalkerLookAwayDuration += DeltaTime;
			if (StalkerLookAwayDuration >= StalkerVanishLookAwayDelay && CurrentEncounterVisibleTime >= MinVisibleDuration)
			{
				VanishFromSight(TEXT("Player looked away after spotting entity"));
				return;
			}
		}
	}

	// Unseen timeout: If entity stands for 30s without ever being seen (player went opposite direction), relocate ahead
	if (!bHasPlayerSpottedStalker)
	{
		StalkerUnseenTime += DeltaTime;
		if (StalkerUnseenTime >= StalkerUnseenRelocateTimeout)
		{
			UE_LOG(LogTemp, Log, TEXT("[BackroomsAI] Stalker unseen timeout expired. Relocating ahead of player."));
			AttemptAppearance();
		}
	}
}
