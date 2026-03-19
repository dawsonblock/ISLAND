// RFSN NPC Schedule System
// Daily routines and time-based behavior for NPCs
// Supports patrol routes, work shifts, sleep schedules, and event triggers

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnNpcSchedule.generated.h"

/**
 * Activity types for scheduled behaviors
 */
UENUM(BlueprintType)
enum class ERfsnActivityType : uint8
{
	Idle UMETA(DisplayName = "Idle"),           // Standing around
	Work UMETA(DisplayName = "Work"),           // At workplace
	Sleep UMETA(DisplayName = "Sleep"),         // Resting
	Eat UMETA(DisplayName = "Eat"),             // Mealtime
	Patrol UMETA(DisplayName = "Patrol"),       // Walking a route
	Socialize UMETA(DisplayName = "Socialize"), // Talking to others
	Trade UMETA(DisplayName = "Trade"),         // Shop/vendor activities
	Guard UMETA(DisplayName = "Guard"),         // Guarding a location
	Travel UMETA(DisplayName = "Travel"),       // Moving between locations
	Custom UMETA(DisplayName = "Custom")        // Custom behavior
};

/**
 * Single schedule entry
 */
USTRUCT(BlueprintType)
struct FRfsnScheduleEntry
{
	GENERATED_BODY()

	/** Start time (0-24 hours) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule", meta = (ClampMin = "0", ClampMax = "24"))
	float StartHour = 0.0f;

	/** End time (0-24 hours) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule", meta = (ClampMin = "0", ClampMax = "24"))
	float EndHour = 24.0f;

	/** Activity type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	ERfsnActivityType Activity = ERfsnActivityType::Idle;

	/** Target location (if applicable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FVector TargetLocation = FVector::ZeroVector;

	/** Use target location? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	bool bHasTargetLocation = false;

	/** Location tag (if using named locations) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FString LocationTag;

	/** Patrol route name (if patrolling) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FString PatrolRouteName;

	/** Custom behavior tag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	FString CustomTag;

	/** Priority (higher = more important, won't be interrupted) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule", meta = (ClampMin = "0", ClampMax = "10"))
	int32 Priority = 5;

	/** Days this applies (0=Sun, 1=Mon, etc. Empty = all days) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule")
	TArray<int32> ActiveDays;

	/** Check if time falls within this entry */
	bool ContainsTime(float Hour) const
	{
		if (StartHour <= EndHour)
		{
			return Hour >= StartHour && Hour < EndHour;
		}
		// Handles overnight spans (e.g., 22:00 - 06:00)
		return Hour >= StartHour || Hour < EndHour;
	}
};

/**
 * Patrol waypoint
 */
USTRUCT(BlueprintType)
struct FRfsnPatrolWaypoint
{
	GENERATED_BODY()

	/** World location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FVector Location = FVector::ZeroVector;

	/** Wait time at this point (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float WaitTime = 2.0f;

	/** Optional action to perform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FString ActionTag;
};

/**
 * Named patrol route
 */
USTRUCT(BlueprintType)
struct FRfsnPatrolRoute
{
	GENERATED_BODY()

	/** Route name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FString RouteName;

	/** Waypoints in order */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	TArray<FRfsnPatrolWaypoint> Waypoints;

	/** Loop the route? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bLoop = true;

	/** Reverse direction when reaching end? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bPingPong = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActivityChanged, ERfsnActivityType, NewActivity, ERfsnActivityType,
                                             PreviousActivity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScheduleLocationReached, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatrolWaypointReached, int32, WaypointIndex);

/**
 * NPC Schedule Component
 * Manages time-based NPC behaviors and routines
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnNpcSchedule : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnNpcSchedule();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Daily schedule entries */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule|Config")
	TArray<FRfsnScheduleEntry> Schedule;

	/** Patrol routes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule|Config")
	TArray<FRfsnPatrolRoute> PatrolRoutes;

	/** Default activity when no schedule matches */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule|Config")
	ERfsnActivityType DefaultActivity = ERfsnActivityType::Idle;

	/** Arrival radius for locations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule|Config")
	float ArrivalRadius = 100.0f;

	/** Movement speed multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule|Config")
	float MoveSpeedMultiplier = 1.0f;

	/** Is schedule enabled? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Schedule|Config")
	bool bScheduleEnabled = true;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Current activity */
	UPROPERTY(BlueprintReadOnly, Category = "Schedule|State")
	ERfsnActivityType CurrentActivity = ERfsnActivityType::Idle;

	/** Current schedule entry index */
	UPROPERTY(BlueprintReadOnly, Category = "Schedule|State")
	int32 CurrentScheduleIndex = -1;

	/** Current patrol waypoint index */
	UPROPERTY(BlueprintReadOnly, Category = "Schedule|State")
	int32 CurrentPatrolIndex = 0;

	/** Is NPC at target location? */
	UPROPERTY(BlueprintReadOnly, Category = "Schedule|State")
	bool bAtTargetLocation = false;

	/** Wait timer for patrol points */
	UPROPERTY(BlueprintReadOnly, Category = "Schedule|State")
	float PatrolWaitTimer = 0.0f;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	/** Called when activity changes */
	UPROPERTY(BlueprintAssignable, Category = "Schedule|Events")
	FOnActivityChanged OnActivityChanged;

	/** Called when NPC reaches scheduled location */
	UPROPERTY(BlueprintAssignable, Category = "Schedule|Events")
	FOnScheduleLocationReached OnLocationReached;

	/** Called when patrol waypoint is reached */
	UPROPERTY(BlueprintAssignable, Category = "Schedule|Events")
	FOnPatrolWaypointReached OnWaypointReached;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Get current schedule entry */
	UFUNCTION(BlueprintPure, Category = "Schedule")
	FRfsnScheduleEntry GetCurrentScheduleEntry() const;

	/** Get current game time (hours 0-24) */
	UFUNCTION(BlueprintPure, Category = "Schedule")
	float GetCurrentGameHour() const;

	/** Get target location for current activity */
	UFUNCTION(BlueprintPure, Category = "Schedule")
	FVector GetCurrentTargetLocation() const;

	/** Get next patrol waypoint */
	UFUNCTION(BlueprintPure, Category = "Schedule")
	FVector GetNextPatrolWaypoint() const;

	/** Check if should be moving toward target */
	UFUNCTION(BlueprintPure, Category = "Schedule")
	bool ShouldMoveToTarget() const;

	/** Interrupt current schedule (temporary override) */
	UFUNCTION(BlueprintCallable, Category = "Schedule")
	void InterruptSchedule(ERfsnActivityType OverrideActivity, float DurationSeconds);

	/** Cancel interrupt and resume schedule */
	UFUNCTION(BlueprintCallable, Category = "Schedule")
	void ResumeSchedule();

	/** Advance to next patrol waypoint */
	UFUNCTION(BlueprintCallable, Category = "Schedule")
	void AdvancePatrolWaypoint();

	/** Get activity as string */
	UFUNCTION(BlueprintPure, Category = "Schedule")
	static FString ActivityToString(ERfsnActivityType Activity);

	/** Get context string for LLM */
	UFUNCTION(BlueprintPure, Category = "Schedule")
	FString GetScheduleContext() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Is currently interrupted? */
	bool bIsInterrupted = false;

	/** Interrupt timer */
	float InterruptTimer = 0.0f;

	/** Interrupt activity */
	ERfsnActivityType InterruptActivity = ERfsnActivityType::Idle;

	/** Patrol direction (1 = forward, -1 = backward for ping-pong) */
	int32 PatrolDirection = 1;

	/** Update current activity from schedule */
	void UpdateActivityFromSchedule();

	/** Find matching schedule entry for current time */
	int32 FindScheduleEntryForTime(float Hour) const;

	/** Check if at target location */
	bool CheckAtTargetLocation() const;

	/** Find patrol route by name */
	const FRfsnPatrolRoute* FindPatrolRoute(const FString& RouteName) const;
};
