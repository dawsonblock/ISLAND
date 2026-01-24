// RFSN Ambient Chatter Implementation

#include "RfsnAmbientChatter.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnLogging.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

URfsnAmbientChatter::URfsnAmbientChatter()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f; // Check once per second
}

void URfsnAmbientChatter::BeginPlay()
{
	Super::BeginPlay();

	// Add some default lines if none configured
	if (ChatterLines.Num() == 0)
	{
		AddChatterLine(TEXT("*sigh*"), ERfsnChatterTrigger::Idle, 1.0f);
		AddChatterLine(TEXT("Another quiet day..."), ERfsnChatterTrigger::Idle, 1.0f);
		AddChatterLine(TEXT("Hmm..."), ERfsnChatterTrigger::Idle, 0.5f);
		AddChatterLine(TEXT("Someone's there!"), ERfsnChatterTrigger::PlayerNearby, 1.0f);
		AddChatterLine(TEXT("Hello?"), ERfsnChatterTrigger::PlayerNearby, 0.8f);
		AddChatterLine(TEXT("Watch yourself!"), ERfsnChatterTrigger::CombatStart, 1.0f);
		AddChatterLine(TEXT("I need help!"), ERfsnChatterTrigger::LowHealth, 1.0f);
	}

	StartIdleChatter();
}

void URfsnAmbientChatter::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnabled)
	{
		return;
	}

	// Idle chatter timer
	if (bIdleChatterActive)
	{
		IdleTimer += DeltaTime;
		if (IdleTimer >= NextIdleTime)
		{
			TriggerChatter(ERfsnChatterTrigger::Idle);
			ResetIdleTimer();
		}
	}

	// Player nearby check
	static float LastPlayerCheck = 0.0f;
	LastPlayerCheck += DeltaTime;
	if (LastPlayerCheck >= 5.0f) // Check every 5 seconds
	{
		LastPlayerCheck = 0.0f;
		if (IsPlayerNearby())
		{
			// Small chance to trigger nearby chatter
			if (FMath::RandRange(0.0f, 1.0f) < 0.1f)
			{
				TriggerChatter(ERfsnChatterTrigger::PlayerNearby);
			}
		}
	}
}

void URfsnAmbientChatter::TriggerChatter(ERfsnChatterTrigger Trigger)
{
	FString Line;

	if (bUseRfsnForChatter)
	{
		// Use RFSN client to generate contextual chatter
		URfsnNpcClientComponent* RfsnClient = GetOwner()->FindComponentByClass<URfsnNpcClientComponent>();
		if (RfsnClient)
		{
			FString Context =
			    FString::Printf(TEXT("[%s] Generate a short %s phrase"), *RfsnClient->NpcName, *RfsnChatterContext);
			RfsnClient->SendPlayerUtterance(Context);
			// Response will come through RFSN events
			return;
		}
	}

	// Use predefined line
	Line = SelectRandomLine(Trigger);
	if (!Line.IsEmpty())
	{
		SayLine(Line);
		OnChatterTriggered.Broadcast(Line, Trigger);
	}
}

void URfsnAmbientChatter::AddChatterLine(const FString& Line, ERfsnChatterTrigger Trigger, float Weight)
{
	FRfsnChatterLine NewLine;
	NewLine.Line = Line;
	NewLine.Trigger = Trigger;
	NewLine.Weight = Weight;
	ChatterLines.Add(NewLine);
}

void URfsnAmbientChatter::SayLine(const FString& Line)
{
	// Get NPC name
	FString NpcName = TEXT("NPC");
	if (URfsnNpcClientComponent* Client = GetOwner()->FindComponentByClass<URfsnNpcClientComponent>())
	{
		NpcName = Client->NpcName;
	}

	RFSN_DIALOGUE_LOG("[%s] (Ambient) %s", *NpcName, *Line);

	// Could also trigger TTS here if available
}

void URfsnAmbientChatter::StartIdleChatter()
{
	bIdleChatterActive = true;
	ResetIdleTimer();
}

void URfsnAmbientChatter::StopIdleChatter()
{
	bIdleChatterActive = false;
}

FString URfsnAmbientChatter::SelectRandomLine(ERfsnChatterTrigger Trigger)
{
	TArray<FRfsnChatterLine> ValidLines;
	float TotalWeight = 0.0f;

	for (const FRfsnChatterLine& Line : ChatterLines)
	{
		if (Line.Trigger == Trigger)
		{
			ValidLines.Add(Line);
			TotalWeight += Line.Weight;
		}
	}

	if (ValidLines.Num() == 0)
	{
		return FString();
	}

	// Weighted random selection
	float Random = FMath::RandRange(0.0f, TotalWeight);
	float Accumulated = 0.0f;

	for (const FRfsnChatterLine& Line : ValidLines)
	{
		Accumulated += Line.Weight;
		if (Random <= Accumulated)
		{
			return Line.Line;
		}
	}

	return ValidLines.Last().Line;
}

void URfsnAmbientChatter::ResetIdleTimer()
{
	IdleTimer = 0.0f;
	NextIdleTime = FMath::RandRange(MinIdleInterval, MaxIdleInterval);
}

bool URfsnAmbientChatter::IsPlayerNearby() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || !PC->GetPawn())
	{
		return false;
	}

	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), PC->GetPawn()->GetActorLocation());
	return Distance <= PlayerDetectionRadius;
}
