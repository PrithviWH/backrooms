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

ABackroomsAIController::ABackroomsAIController()
	: CurrentState(EBackroomsAIState::IdleWander)
	, TargetPlayer(nullptr)
	, bIsChasing(false)
	, LostSightSearchDuration(18.0f)   // Stalker long memory: 18s search before giving up
	, SearchLingerRadius(500.0f)       // Lingers tightly near the last seen location
	, WanderRadius(1200.0f)
	, WanderSpeed(180.0f)              // Quiet, deliberate stalker wander
	, SearchSpeed(240.0f)              // Deliberate, cautious stalking speed
	, ChaseSpeed(450.0f)               // Tense pursuit speed (player sprint is 600, walk is 300)
	, ChaseSound(nullptr)
	, LastKnownPlayerLocation(FVector::ZeroVector)
{
	PrimaryActorTick.bCanEverTick = true;
	bStartAILogicOnPossess = true;
	bStopAILogicOnUnposses = true;

	// Create Perception Component
	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SetPerceptionComponent(*Perception);

	// Configure Sight Sense for "Stealthy Stalker"
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1200.0f;                    // Narrower range: ~12m
	SightConfig->LoseSightRadius = 1800.0f;                // Keep wider than SightRadius so it tracks through corners
	SightConfig->PeripheralVisionAngleDegrees = 45.0f;     // Narrow 45° cone: player can evade by staying outside forward view
	SightConfig->SetMaxAge(18.0f);
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
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ABackroomsAIController::OnTargetPerceptionUpdated);
	}

	CurrentState = EBackroomsAIState::IdleWander;
	SetEntityMovementSpeed(WanderSpeed);
	UpdateBlackboardKeys();

	// Schedule initial wander
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(WanderWaitTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (GetPawn() && !GetPawn()->IsActorBeingDestroyed())
			{
				MoveToRandomReachablePoint(WanderRadius);
			}
		}), 1.5f, false);
	}
}

void ABackroomsAIController::OnUnPossess()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LostSightTimerHandle);
		World->GetTimerManager().ClearTimer(WanderWaitTimerHandle);
		World->GetTimerManager().ClearTimer(ChaseNextTickTimerHandle);
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

	if (Stimulus.WasSuccessfullySensed())
	{
		// SIGHT ACQUIRED -> Transition to Chase
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(LostSightTimerHandle);
			World->GetTimerManager().ClearTimer(WanderWaitTimerHandle);
			World->GetTimerManager().ClearTimer(ChaseNextTickTimerHandle);
		}

		// Play loud audio cue ONLY on entering Chase (stealthy silent during wander/search)
		if (CurrentState != EBackroomsAIState::Chase && ChaseSound && GetPawn())
		{
			UGameplayStatics::PlaySoundAtLocation(this, ChaseSound, GetPawn()->GetActorLocation());
		}

		CurrentState = EBackroomsAIState::Chase;
		TargetPlayer = Actor;
		bIsChasing = true;
		LastKnownPlayerLocation = Stimulus.StimulusLocation;

		SetEntityMovementSpeed(ChaseSpeed);
		SetFocus(Actor);
		MoveToActor(Actor, 80.0f);

		UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Player detected in forward 45 deg cone! Entering Chase."));

		UpdateBlackboardKeys();
	}
	else
	{
		// SIGHT LOST -> Transition to Search (Long memory: linger near last known location)
		ClearFocus(EAIFocusPriority::Gameplay);
		LastKnownPlayerLocation = Stimulus.StimulusLocation;
		bIsChasing = false;
		CurrentState = EBackroomsAIState::Search;

		SetEntityMovementSpeed(SearchSpeed);

		// Move directly toward last known location
		MoveToLocation(LastKnownPlayerLocation, 60.0f);

		UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Player lost sight! Searching near last known location for %.1f s."), LostSightSearchDuration);

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
	// Search timer expired without re-acquiring player -> Resume quiet idle wander
	TargetPlayer = nullptr;
	bIsChasing = false;
	CurrentState = EBackroomsAIState::IdleWander;
	ClearFocus(EAIFocusPriority::Gameplay);

	SetEntityMovementSpeed(WanderSpeed);
	UpdateBlackboardKeys();

	UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Search timer expired. Resuming silent Idle Wander."));

	MoveToRandomReachablePoint(WanderRadius);
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
