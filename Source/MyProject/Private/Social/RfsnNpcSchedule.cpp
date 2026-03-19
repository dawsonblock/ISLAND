// RFSN NPC Schedule Implementation

#include "RfsnNpcSchedule.h"
#include "RfsnLogging.h"
#include "Kismet/GameplayStatics.h"

URfsnNpcSchedule::URfsnNpcSchedule()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f; // Update once per second
}

void URfsnNpcSchedule::BeginPlay()
{
	Super::BeginPlay();

	// Initial schedule update
	UpdateActivityFromSchedule();

	RFSN_LOG(TEXT("NpcSchedule initialized for %s with %d entries"), *GetOwner()->GetName(), Schedule.Num());
}

void URfsnNpcSchedule::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bScheduleEnabled)
	{
		return;
	}

	// Handle interrupt timer
	if (bIsInterrupted)
	{
		InterruptTimer -= DeltaTime;
		if (InterruptTimer <= 0.0f)
		{
			ResumeSchedule();
		}
		return;
	}

	// Update activity based on schedule
	UpdateActivityFromSchedule();

	// Handle patrol wait timer
	if (CurrentActivity == ERfsnActivityType::Patrol && bAtTargetLocation)
	{
		PatrolWaitTimer -= DeltaTime;
		if (PatrolWaitTimer <= 0.0f)
		{
			AdvancePatrolWaypoint();
		}
	}

	// Check if at target
	bAtTargetLocation = CheckAtTargetLocation();
}

void URfsnNpcSchedule::UpdateActivityFromSchedule()
{
	float CurrentHour = GetCurrentGameHour();
	int32 NewIndex = FindScheduleEntryForTime(CurrentHour);

	if (NewIndex != CurrentScheduleIndex)
	{
		CurrentScheduleIndex = NewIndex;
		ERfsnActivityType PreviousActivity = CurrentActivity;

		if (NewIndex >= 0 && NewIndex < Schedule.Num())
		{
			CurrentActivity = Schedule[NewIndex].Activity;

			// Reset patrol index when starting new patrol
			if (CurrentActivity == ERfsnActivityType::Patrol)
			{
				CurrentPatrolIndex = 0;
				PatrolDirection = 1;
			}
		}
		else
		{
			CurrentActivity = DefaultActivity;
		}

		if (CurrentActivity != PreviousActivity)
		{
			OnActivityChanged.Broadcast(CurrentActivity, PreviousActivity);
			RFSN_LOG(TEXT("%s activity changed: %s -> %s"), *GetOwner()->GetName(), *ActivityToString(PreviousActivity),
			         *ActivityToString(CurrentActivity));
		}
	}
}

int32 URfsnNpcSchedule::FindScheduleEntryForTime(float Hour) const
{
	int32 BestMatch = -1;
	int32 HighestPriority = -1;

	// Get current day of week
	int32 CurrentDay = 0; // TODO: Get from game time system

	for (int32 i = 0; i < Schedule.Num(); ++i)
	{
		const FRfsnScheduleEntry& Entry = Schedule[i];

		// Check if day matches (empty = all days)
		if (Entry.ActiveDays.Num() > 0 && !Entry.ActiveDays.Contains(CurrentDay))
		{
			continue;
		}

		// Check if time matches
		if (Entry.ContainsTime(Hour))
		{
			if (Entry.Priority > HighestPriority)
			{
				HighestPriority = Entry.Priority;
				BestMatch = i;
			}
		}
	}

	return BestMatch;
}

float URfsnNpcSchedule::GetCurrentGameHour() const
{
	// Get time from game
	// Default: use real-time seconds mapped to 24h cycle (1 game day = 24 real minutes)
	float Seconds = GetWorld() ? UGameplayStatics::GetTimeSeconds(GetWorld()) : 0.0f;
	float Hours = FMath::Fmod(Seconds / 60.0f, 24.0f); // 60 sec = 1 game hour

	return Hours;
}

FRfsnScheduleEntry URfsnNpcSchedule::GetCurrentScheduleEntry() const
{
	if (CurrentScheduleIndex >= 0 && CurrentScheduleIndex < Schedule.Num())
	{
		return Schedule[CurrentScheduleIndex];
	}

	return FRfsnScheduleEntry();
}

FVector URfsnNpcSchedule::GetCurrentTargetLocation() const
{
	if (CurrentActivity == ERfsnActivityType::Patrol)
	{
		return GetNextPatrolWaypoint();
	}

	FRfsnScheduleEntry Entry = GetCurrentScheduleEntry();
	if (Entry.bHasTargetLocation)
	{
		return Entry.TargetLocation;
	}

	return GetOwner()->GetActorLocation();
}

FVector URfsnNpcSchedule::GetNextPatrolWaypoint() const
{
	FRfsnScheduleEntry Entry = GetCurrentScheduleEntry();
	const FRfsnPatrolRoute* Route = FindPatrolRoute(Entry.PatrolRouteName);

	if (!Route || Route->Waypoints.Num() == 0)
	{
		return GetOwner()->GetActorLocation();
	}

	int32 SafeIndex = FMath::Clamp(CurrentPatrolIndex, 0, Route->Waypoints.Num() - 1);
	return Route->Waypoints[SafeIndex].Location;
}

bool URfsnNpcSchedule::ShouldMoveToTarget() const
{
	if (!bScheduleEnabled || bIsInterrupted)
	{
		return false;
	}

	// Activities that require movement
	switch (CurrentActivity)
	{
	case ERfsnActivityType::Patrol:
	case ERfsnActivityType::Travel:
		return !bAtTargetLocation;
	case ERfsnActivityType::Work:
	case ERfsnActivityType::Sleep:
	case ERfsnActivityType::Eat:
	case ERfsnActivityType::Guard:
	case ERfsnActivityType::Trade:
		return !bAtTargetLocation;
	default:
		return false;
	}
}

void URfsnNpcSchedule::InterruptSchedule(ERfsnActivityType OverrideActivity, float DurationSeconds)
{
	bIsInterrupted = true;
	InterruptActivity = OverrideActivity;
	InterruptTimer = DurationSeconds;

	ERfsnActivityType Previous = CurrentActivity;
	CurrentActivity = OverrideActivity;
	OnActivityChanged.Broadcast(CurrentActivity, Previous);

	RFSN_LOG(TEXT("%s schedule interrupted for %.1f seconds"), *GetOwner()->GetName(), DurationSeconds);
}

void URfsnNpcSchedule::ResumeSchedule()
{
	bIsInterrupted = false;
	InterruptTimer = 0.0f;

	// Force update to current schedule
	CurrentScheduleIndex = -1;
	UpdateActivityFromSchedule();

	RFSN_LOG(TEXT("%s schedule resumed"), *GetOwner()->GetName());
}

void URfsnNpcSchedule::AdvancePatrolWaypoint()
{
	FRfsnScheduleEntry Entry = GetCurrentScheduleEntry();
	const FRfsnPatrolRoute* Route = FindPatrolRoute(Entry.PatrolRouteName);

	if (!Route || Route->Waypoints.Num() == 0)
	{
		return;
	}

	bAtTargetLocation = false;
	OnWaypointReached.Broadcast(CurrentPatrolIndex);

	// Advance index
	CurrentPatrolIndex += PatrolDirection;

	// Handle end of route
	if (CurrentPatrolIndex >= Route->Waypoints.Num())
	{
		if (Route->bPingPong)
		{
			PatrolDirection = -1;
			CurrentPatrolIndex = Route->Waypoints.Num() - 2;
		}
		else if (Route->bLoop)
		{
			CurrentPatrolIndex = 0;
		}
		else
		{
			CurrentPatrolIndex = Route->Waypoints.Num() - 1;
		}
	}
	else if (CurrentPatrolIndex < 0)
	{
		if (Route->bPingPong)
		{
			PatrolDirection = 1;
			CurrentPatrolIndex = 1;
		}
		else
		{
			CurrentPatrolIndex = 0;
		}
	}

	// Set wait timer for next waypoint
	if (CurrentPatrolIndex >= 0 && CurrentPatrolIndex < Route->Waypoints.Num())
	{
		PatrolWaitTimer = Route->Waypoints[CurrentPatrolIndex].WaitTime;
	}
}

bool URfsnNpcSchedule::CheckAtTargetLocation() const
{
	FVector Target = GetCurrentTargetLocation();
	FVector Current = GetOwner()->GetActorLocation();

	float Distance = FVector::Dist2D(Current, Target);
	return Distance <= ArrivalRadius;
}

const FRfsnPatrolRoute* URfsnNpcSchedule::FindPatrolRoute(const FString& RouteName) const
{
	return PatrolRoutes.FindByPredicate([&RouteName](const FRfsnPatrolRoute& Route)
	                                    { return Route.RouteName.Equals(RouteName, ESearchCase::IgnoreCase); });
}

FString URfsnNpcSchedule::ActivityToString(ERfsnActivityType Activity)
{
	switch (Activity)
	{
	case ERfsnActivityType::Idle:
		return TEXT("Idle");
	case ERfsnActivityType::Work:
		return TEXT("Working");
	case ERfsnActivityType::Sleep:
		return TEXT("Sleeping");
	case ERfsnActivityType::Eat:
		return TEXT("Eating");
	case ERfsnActivityType::Patrol:
		return TEXT("Patrolling");
	case ERfsnActivityType::Socialize:
		return TEXT("Socializing");
	case ERfsnActivityType::Trade:
		return TEXT("Trading");
	case ERfsnActivityType::Guard:
		return TEXT("Guarding");
	case ERfsnActivityType::Travel:
		return TEXT("Traveling");
	default:
		return TEXT("Custom");
	}
}

FString URfsnNpcSchedule::GetScheduleContext() const
{
	FString Context = FString::Printf(TEXT("Currently %s."), *ActivityToString(CurrentActivity));

	float Hour = GetCurrentGameHour();
	int32 WholeHour = FMath::FloorToInt(Hour);
	int32 Minutes = FMath::FloorToInt((Hour - WholeHour) * 60.0f);

	Context += FString::Printf(TEXT(" Current time: %02d:%02d."), WholeHour, Minutes);

	// Add next activity if known
	for (const FRfsnScheduleEntry& Entry : Schedule)
	{
		if (Entry.StartHour > Hour)
		{
			int32 NextHour = FMath::FloorToInt(Entry.StartHour);
			FString NextActivity = ActivityToString(Entry.Activity);
			Context += FString::Printf(TEXT(" Will %s at %02d:00."), *NextActivity.ToLower(), NextHour);
			break;
		}
	}

	return Context;
}
