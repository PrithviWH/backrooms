// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BackroomsAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBehaviorTree;
class UBlackboardData;
class USoundBase;

UENUM(BlueprintType)
enum class EBackroomsAIState : uint8
{
	IdleWander UMETA(DisplayName = "Idle Wander"),
	Chase      UMETA(DisplayName = "Chase"),
	Search     UMETA(DisplayName = "Search")
};

/**
 * Authoritative AI Controller for Backrooms Entity / "Stealthy Stalker" AI.
 * Implements narrow-angle stealth detection, silent stalking/wandering, aggressive chasing on sight,
 * and an extended search phase where the entity lingers near the last known player location.
 * Actively writes and reads BB_BackroomsEntity Blackboard keys for state and debug telemetry.
 */
UCLASS()
class BACKROOMS_API ABackroomsAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABackroomsAIController();

	/** Current AI operational state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|State")
	EBackroomsAIState CurrentState;

	/** Optional BehaviorTree asset to run on possess */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|BehaviorTree")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	/** Optional Blackboard asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|BehaviorTree")
	TObjectPtr<UBlackboardData> BlackboardAsset;

	/** Target player currently detected in sight */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|State")
	TObjectPtr<AActor> TargetPlayer;

	/** Whether entity is currently chasing the player */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|State")
	bool bIsChasing;

	/** Seconds to search the last known location before resuming idle wander (15-20s for Stalker) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Parameters")
	float LostSightSearchDuration;

	/** Radius around last-known player location to search while in Search state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Parameters")
	float SearchLingerRadius;

	/** Radius for finding random reachable nav points during Idle Wander */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Parameters")
	float WanderRadius;

	/** Movement speed while wandering (quiet, 180 cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Parameters")
	float WanderSpeed;

	/** Movement speed while chasing (450 cm/s, slower than player sprint for dread/evasion) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Parameters")
	float ChaseSpeed;

	/** Audio played ONLY when transitioning into active chase (silence during wander/search) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Audio")
	TObjectPtr<USoundBase> ChaseSound;

	/** Sight configuration for AI Perception */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** Moves entity to a random reachable location on the NavMesh around its current position */
	UFUNCTION(BlueprintCallable, Category = "AI|Actions")
	void MoveToRandomReachablePoint(float Radius = 1200.0f);

	/** Moves entity to a random reachable location around a specific center point (for searching) */
	UFUNCTION(BlueprintCallable, Category = "AI|Actions")
	void MoveToReachablePointNear(const FVector& Center, float Radius = 500.0f);

	/** Callback when AI Perception updates for an actor */
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** Called when the lost-sight search timer expires to resume idle wandering */
	UFUNCTION(BlueprintCallable, Category = "AI|Actions")
	void OnLoseSightTimerExpired();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	FTimerHandle LostSightTimerHandle;
	FTimerHandle WanderWaitTimerHandle;
	FVector LastKnownPlayerLocation;

	void SetEntityMovementSpeed(float Speed);
	void UpdateBlackboardKeys();
};
