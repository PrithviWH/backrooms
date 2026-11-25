// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsAIController.h"
#include "NavigationSystem.h"
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

	if (UAIPerceptionComponent* Perception = GetAIPerceptionComponent())
	{
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ABackroomsAIController::OnTargetPerceptionUpdated);
	}

	CurrentState = EBackroomsAIState::IdleWander;
	SetEntityMovementSpeed(WanderSpeed);
	UpdateBlackboardKeys();

	// Schedule initial wander
	GetWorld()->GetTimerManager().SetTimer(WanderWaitTimerHandle, [this]()
	{
		MoveToRandomReachablePoint(WanderRadius);
	}, 1.5f, false);
}

void ABackroomsAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if !UE_BUILD_SHIPPING
	if (APawn* ControlledPawn = GetPawn())
	{
		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			// Read AIState directly from the Blackboard component (justifying BB usage)
			uint8 StateVal = BB->GetValueAsEnum(TEXT("AIState"));
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
				StateStr = FString::Printf(TEXT("SEARCHING (%.1fs left)"), GetWorld()->GetTimerManager().GetTimerRemaining(LostSightTimerHandle));
				StateColor = FColor::Yellow;
				break;
			}

			FVector TextLocation = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 110.0f);
			DrawDebugString(GetWorld(), TextLocation, FString::Printf(TEXT("[BB_AIState: %s]"), *StateStr), nullptr, StateColor, 0.0f, false, 1.2f);
		}
	}
#endif
}

void ABackroomsAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (CurrentState == EBackroomsAIState::Chase && TargetPlayer)
	{
		// Continuously follow target in active chase
		MoveToActor(TargetPlayer, 80.0f);
	}
	else if (CurrentState == EBackroomsAIState::Search)
	{
		// Lingering search: wander specifically near last known player location
		GetWorld()->GetTimerManager().SetTimer(WanderWaitTimerHandle, [this]()
		{
			if (CurrentState == EBackroomsAIState::Search)
			{
				MoveToReachablePointNear(LastKnownPlayerLocation, SearchLingerRadius);
			}
		}, FMath::RandRange(1.0f, 2.5f), false);
	}
	else if (CurrentState == EBackroomsAIState::IdleWander)
	{
		// Passive corridor wander
		GetWorld()->GetTimerManager().SetTimer(WanderWaitTimerHandle, [this]()
		{
			if (CurrentState == EBackroomsAIState::IdleWander)
			{
				MoveToRandomReachablePoint(WanderRadius);
			}
		}, FMath::RandRange(2.0f, 4.0f), false);
	}
}

void ABackroomsAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || Actor == GetPawn())
	{
		return;
	}

	ACharacter* PlayerChar = Cast<ACharacter>(Actor);
	if (!PlayerChar || !PlayerChar->IsPlayerControlled())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// SIGHT ACQUIRED -> Transition to Chase
		GetWorld()->GetTimerManager().ClearTimer(LostSightTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(WanderWaitTimerHandle);

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
		SetFocus(TargetPlayer);
		MoveToActor(TargetPlayer, 80.0f);

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

		// Move directly toward last known location
		MoveToLocation(LastKnownPlayerLocation, 60.0f);

		UE_LOG(LogTemp, Warning, TEXT("[BackroomsAI] Player lost sight! Searching near last known location for %.1f s."), LostSightSearchDuration);

		UpdateBlackboardKeys();

		// Start long memory search timer (15-20s)
		GetWorld()->GetTimerManager().SetTimer(LostSightTimerHandle, this, &ABackroomsAIController::OnLoseSightTimerExpired, LostSightSearchDuration, false);
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
	if (!ControlledPawn)
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
			BB->SetValueAsVector(TEXT("TargetLocation"), RandomLocation.Location);
		}
	}
}

void ABackroomsAIController::MoveToReachablePointNear(const FVector& Center, float Radius)
{
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
			BB->SetValueAsVector(TEXT("TargetLocation"), RandomLocation.Location);
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
		BB->SetValueAsEnum(TEXT("AIState"), static_cast<uint8>(CurrentState));

		if (TargetPlayer)
		{
			BB->SetValueAsObject(TEXT("TargetActor"), TargetPlayer);
			BB->SetValueAsBool(TEXT("HasTarget"), true);
		}
		else
		{
			BB->ClearValue(TEXT("TargetActor"));
			BB->SetValueAsBool(TEXT("HasTarget"), false);
		}

		BB->SetValueAsVector(TEXT("TargetLocation"), LastKnownPlayerLocation);
	}
}
