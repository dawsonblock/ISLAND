// RFSN NPC Barks Implementation

#include "RfsnNpcBarks.h"
#include "RfsnLogging.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

URfsnNpcBarks::URfsnNpcBarks()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f;
}

void URfsnNpcBarks::BeginPlay()
{
	Super::BeginPlay();

	if (Barks.Num() == 0)
	{
		SetupDefaultBarks();
	}

	// Randomize initial idle timer
	IdleTimer = FMath::RandRange(IdleBarkInterval * 0.5f, IdleBarkInterval);

	RFSN_LOG(TEXT("NpcBarks initialized for %s with %d barks"), *GetOwner()->GetName(), Barks.Num());
}

void URfsnNpcBarks::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Idle bark timer
	if (bEnableIdleBarks && !bIsBarking)
	{
		IdleTimer -= DeltaTime;
		if (IdleTimer <= 0.0f)
		{
			IdleTimer = IdleBarkInterval + FMath::RandRange(-10.0f, 10.0f);
			TryBark(ERfsnBarkTrigger::Idle);
		}
	}

	// Clear bark state
	if (bIsBarking)
	{
		bIsBarking = false;
	}
}

bool URfsnNpcBarks::TryBark(ERfsnBarkTrigger Trigger, bool bForce)
{
	if (!bForce)
	{
		if (!CanBark())
		{
			return false;
		}

		if (FMath::FRand() > BarkChance)
		{
			return false;
		}

		// Check if player is in range
		if (!IsPlayerInRange())
		{
			return false;
		}
	}

	FRfsnBark* SelectedBark = SelectBark(Trigger);
	if (!SelectedBark)
	{
		return false;
	}

	// Update usage
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	SelectedBark->LastUsedTime = CurrentTime;
	LastBarkTime = CurrentTime;
	CurrentBark = SelectedBark->Text;
	bIsBarking = true;

	OnBarkTriggered.Broadcast(Trigger, SelectedBark->Text);

	RFSN_LOG(TEXT("%s barks: %s"), *GetOwner()->GetName(), *SelectedBark->Text);
	return true;
}

void URfsnNpcBarks::SayBark(const FString& Text)
{
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	LastBarkTime = CurrentTime;
	CurrentBark = Text;
	bIsBarking = true;

	OnBarkTriggered.Broadcast(ERfsnBarkTrigger::Custom, Text);
}

bool URfsnNpcBarks::TryCustomBark(const FString& CustomTag)
{
	if (!CanBark())
	{
		return false;
	}

	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	for (FRfsnBark& Bark : Barks)
	{
		if (Bark.Trigger == ERfsnBarkTrigger::Custom && Bark.CustomTag.Equals(CustomTag, ESearchCase::IgnoreCase) &&
		    Bark.IsAvailable(CurrentTime))
		{
			Bark.LastUsedTime = CurrentTime;
			LastBarkTime = CurrentTime;
			CurrentBark = Bark.Text;
			bIsBarking = true;

			OnBarkTriggered.Broadcast(ERfsnBarkTrigger::Custom, Bark.Text);
			return true;
		}
	}

	return false;
}

void URfsnNpcBarks::AddBark(ERfsnBarkTrigger Trigger, const FString& Text, int32 Priority, float InCooldown)
{
	FRfsnBark Bark;
	Bark.Trigger = Trigger;
	Bark.Text = Text;
	Bark.Priority = Priority;
	Bark.Cooldown = InCooldown;
	Barks.Add(Bark);
}

void URfsnNpcBarks::ClearBarks(ERfsnBarkTrigger Trigger)
{
	Barks.RemoveAll([Trigger](const FRfsnBark& Bark) { return Bark.Trigger == Trigger; });
}

FString URfsnNpcBarks::GetRandomBark(ERfsnBarkTrigger Trigger) const
{
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	TArray<const FRfsnBark*> Available;
	for (const FRfsnBark& Bark : Barks)
	{
		if (Bark.Trigger == Trigger && Bark.IsAvailable(CurrentTime))
		{
			Available.Add(&Bark);
		}
	}

	if (Available.Num() == 0)
	{
		return TEXT("");
	}

	return Available[FMath::RandRange(0, Available.Num() - 1)]->Text;
}

bool URfsnNpcBarks::CanBark() const
{
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return CurrentTime - LastBarkTime >= GlobalCooldown;
}

bool URfsnNpcBarks::IsPlayerInRange() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || !PC->GetPawn())
	{
		return false;
	}

	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), PC->GetPawn()->GetActorLocation());
	return Distance <= HearingRange;
}

void URfsnNpcBarks::SetupDefaultBarks()
{
	Barks.Empty();

	// Idle
	AddBark(ERfsnBarkTrigger::Idle, TEXT("*sigh*"), 3, 60.0f);
	AddBark(ERfsnBarkTrigger::Idle, TEXT("Hmm..."), 3, 60.0f);
	AddBark(ERfsnBarkTrigger::Idle, TEXT("What a day..."), 3, 60.0f);
	AddBark(ERfsnBarkTrigger::Idle, TEXT("Stay alert..."), 4, 60.0f);

	// Greetings
	AddBark(ERfsnBarkTrigger::Greeting, TEXT("Hey there."), 5, 30.0f);
	AddBark(ERfsnBarkTrigger::Greeting, TEXT("Oh, hello."), 5, 30.0f);
	AddBark(ERfsnBarkTrigger::Greeting, TEXT("You again?"), 4, 30.0f);

	// Farewells
	AddBark(ERfsnBarkTrigger::Farewell, TEXT("See you around."), 5, 30.0f);
	AddBark(ERfsnBarkTrigger::Farewell, TEXT("Take care."), 5, 30.0f);
	AddBark(ERfsnBarkTrigger::Farewell, TEXT("Stay safe out there."), 5, 30.0f);

	// Player near
	AddBark(ERfsnBarkTrigger::PlayerNear, TEXT("Hmm?"), 4, 30.0f);
	AddBark(ERfsnBarkTrigger::PlayerNear, TEXT("Need something?"), 5, 30.0f);

	// Player leave
	AddBark(ERfsnBarkTrigger::PlayerLeave, TEXT("Leaving already?"), 4, 30.0f);
	AddBark(ERfsnBarkTrigger::PlayerLeave, TEXT("Watch yourself."), 4, 30.0f);

	// Combat
	AddBark(ERfsnBarkTrigger::Combat, TEXT("Get ready!"), 7, 15.0f);
	AddBark(ERfsnBarkTrigger::Combat, TEXT("Here they come!"), 7, 15.0f);
	AddBark(ERfsnBarkTrigger::Combat, TEXT("Fight!"), 6, 15.0f);

	// Danger
	AddBark(ERfsnBarkTrigger::Danger, TEXT("Watch out!"), 8, 10.0f);
	AddBark(ERfsnBarkTrigger::Danger, TEXT("Something's wrong..."), 6, 20.0f);
	AddBark(ERfsnBarkTrigger::Danger, TEXT("Did you hear that?"), 6, 20.0f);

	// Weather
	AddBark(ERfsnBarkTrigger::Weather, TEXT("This weather..."), 3, 120.0f);
	AddBark(ERfsnBarkTrigger::Weather, TEXT("Hope it clears up."), 3, 120.0f);

	// Time of day
	AddBark(ERfsnBarkTrigger::TimeOfDay, TEXT("Another day begins."), 3, 300.0f);
	AddBark(ERfsnBarkTrigger::TimeOfDay, TEXT("Getting dark..."), 4, 300.0f);
	AddBark(ERfsnBarkTrigger::TimeOfDay, TEXT("Night falls."), 4, 300.0f);

	// Pain
	AddBark(ERfsnBarkTrigger::Pain, TEXT("Ugh!"), 8, 5.0f);
	AddBark(ERfsnBarkTrigger::Pain, TEXT("That hurt!"), 7, 5.0f);

	// Victory
	AddBark(ERfsnBarkTrigger::Victory, TEXT("Got 'em!"), 6, 20.0f);
	AddBark(ERfsnBarkTrigger::Victory, TEXT("That's that."), 5, 20.0f);

	// Frustrated
	AddBark(ERfsnBarkTrigger::Frustrated, TEXT("This is getting old..."), 4, 60.0f);
	AddBark(ERfsnBarkTrigger::Frustrated, TEXT("*grumbles*"), 3, 60.0f);
}

FRfsnBark* URfsnNpcBarks::SelectBark(ERfsnBarkTrigger Trigger)
{
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	TArray<FRfsnBark*> Available;
	for (FRfsnBark& Bark : Barks)
	{
		if (Bark.Trigger == Trigger && Bark.IsAvailable(CurrentTime))
		{
			Available.Add(&Bark);
		}
	}

	if (Available.Num() == 0)
	{
		return nullptr;
	}

	// Weight by priority
	int32 TotalPriority = 0;
	for (const FRfsnBark* Bark : Available)
	{
		TotalPriority += Bark->Priority;
	}

	int32 Roll = FMath::RandRange(1, TotalPriority);
	int32 Cumulative = 0;

	for (FRfsnBark* Bark : Available)
	{
		Cumulative += Bark->Priority;
		if (Roll <= Cumulative)
		{
			return Bark;
		}
	}

	return Available.Last();
}
