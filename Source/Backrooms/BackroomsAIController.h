// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BackroomsAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBehaviorTree;
class UBlackboardData;
class USoundBase;

UENUM(BlueprintType)
enum class EBackroomsAIState : uint8
{
	DistantStalker UMETA(DisplayName = "Distant Stalker"),
	IdleWander     UMETA(DisplayName = "Idle Wander"),
	Chase          UMETA(DisplayName = "Chase"),
	Search         UMETA(DisplayName = "Search")
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

	/** Blackboard key selector for AI operational state */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
	FBlackboardKeySelector AIStateKey;

	/** Blackboard key selector for target player actor */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** Blackboard key selector for whether entity has an active target */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
	FBlackboardKeySelector HasTargetKey;

	/** Blackboard key selector for target/wander location */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Blackboard")
	FBlackboardKeySelector TargetLocationKey;

	/** Optional BehaviorTree asset to run on possess */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|BehaviorTree")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	/** Optional Blackboard asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|BehaviorTree")
	TObjectPtr<UBlackboardData> BlackboardAsset;

	/** Target player currently detected in sight */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|State")
	TWeakObjectPtr<AActor> TargetPlayer;

	/** Helper to get valid target player actor */
	UFUNCTION(BlueprintPure, Category = "AI|State")
	AActor* GetTargetPlayer() const { return TargetPlayer.Get(); }

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

	/** Movement speed while searching (deliberate stalking, 240 cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Parameters")
	float SearchSpeed;

	/** Movement speed while chasing (450 cm/s, slower than player sprint for dread/evasion) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Parameters")
	float ChaseSpeed;

	/** Audio played ONLY when transitioning into active chase (silence during wander/search) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Audio")
	TObjectPtr<USoundBase> ChaseSound;

	// --- Distant Stalker Encounter Timing & Lifecycle Parameters ---

	/** Minimum distance (cm) for distant stalking sightings (default: 1800cm / 18m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float StalkingMinDistance;

	/** Maximum distance (cm) for distant stalking sightings (default: 3400cm / 34m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float StalkingMaxDistance;

	/** Minimum appearance interval (seconds) after an encounter ends (default: 18.0s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float AppearanceIntervalMin;

	/** Maximum appearance interval (seconds) after an encounter ends (default: 35.0s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float AppearanceIntervalMax;

	/** Minimum visible duration (seconds) before entity is allowed to vanish on look-away (default: 2.0s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float MinVisibleDuration;

	/** Maximum visible duration (seconds) before entity vanishes if player keeps staring (default: 15.0s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float MaxVisibleDuration;

	/** Seconds player must look away after spotting before entity vanishes into darkness (default: 0.5s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float StalkerVanishLookAwayDelay;

	/** Seconds player must look directly at distant stalker before spotting is confirmed (default: 0.6s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float StalkerSpottedTimeThreshold;

	/** Minimum delay (seconds) before retrying if no valid distant stalking location was found (default: 2.0s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float FailedSpawnRetryDelayMin;

	/** Maximum delay (seconds) before retrying if no valid distant stalking location was found (default: 4.0s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float FailedSpawnRetryDelayMax;

	/** Maximum seconds entity will wait at a vantage point if player never looks towards it before relocating (default: 30.0s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float StalkerUnseenRelocateTimeout;

	/** Close proximity threshold (cm): if player rushes directly towards stalker within this distance, it vanishes into darkness (default: 850cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Stalking")
	float StalkerCloseProximityThreshold;

	/** Whether player has spotted the current distant stalker */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Stalking")
	bool bHasPlayerSpottedStalker;

	/** Whether entity is currently vanished / waiting for next stalking sighting */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Stalking")
	bool bIsStalkerVanished;

	/** Total time current encounter has been visible */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Stalking")
	float CurrentEncounterVisibleTime;

	/** Validates candidate location with downward ground line trace and capsule overlap test */
	bool ValidateAndAlignGroundLocation(const FVector& InNavLocation, FVector& OutSpawnLocation);

	/** Finds an optimal distant vantage point visible down a corridor or intersection from player's view */
	UFUNCTION(BlueprintCallable, Category = "AI|Stalking")
	bool FindDistantStalkingLocation(const AActor* InPlayerActor, FVector& OutLocation, FRotator& OutRotation);

	/** Teleports entity to a new distant stalking vantage point facing the player */
	UFUNCTION(BlueprintCallable, Category = "AI|Stalking")
	bool RelocateToDistantStalkingVantage();

	/** Attempts to spawn the entity at a distant vantage point; retries automatically after short delay on failure */
	UFUNCTION(BlueprintCallable, Category = "AI|Stalking")
	void AttemptAppearance();

	/** Makes the entity vanish into darkness after being spotted and looked away from */
	UFUNCTION(BlueprintCallable, Category = "AI|Stalking")
	void VanishFromSight(const FString& Reason = TEXT("Player looked away"));

	// --- Evasion & Disengagement Parameters ---

	/** Seconds player must sprint away in opposite direction before entity gives up pursuit (default: 1.2s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Evasion")
	float EvasionBreakTime;

	/** Minimum distance (cm) between player and monster for sprinting away to trigger disengagement (default: 700cm / 7m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Evasion")
	float EvasionDistanceThreshold;

	/** Cooldown (seconds) after disengaging before entity can re-acquire target via footsteps (default: 6.0s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Evasion")
	float EvasionCooldownDuration;

	/** Maximum continuous chase duration (seconds) before entity fatigues and gives up (default: 12.0s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Evasion")
	float MaxChaseDuration;

	/** Smartly ceases chase, halts movement, and transitions to search/idle */
	UFUNCTION(BlueprintCallable, Category = "AI|Actions")
	void SmartDisengage(const FString& Reason = TEXT("Player Evaded"));

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
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	FTimerHandle LostSightTimerHandle;
	FTimerHandle WanderWaitTimerHandle;
	FTimerHandle ChaseNextTickTimerHandle;
	FTimerHandle StalkerAppearanceTimerHandle;
	FVector LastKnownPlayerLocation;

	void SetEntityMovementSpeed(float Speed);
	void UpdateBlackboardKeys();
	void UpdateEvasionCheck(float DeltaTime);
	void UpdateDistantStalker(float DeltaTime);

	float PlayerEvasionTime;
	float ChaseDurationTimer;
	float EvasionReAggroCooldownTimer;
	float StalkerLookDuration;
	float StalkerLookAwayDuration;
	float StalkerUnseenTime;
};
