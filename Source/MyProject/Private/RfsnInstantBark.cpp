// RFSN Instant Bark System - Implementation

#include "RfsnInstantBark.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnLogging.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

URfsnInstantBark::URfsnInstantBark()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URfsnInstantBark::BeginPlay()
{
	Super::BeginPlay();

	// Create audio component
	AActor* Owner = GetOwner();
	if (Owner)
	{
		AudioComponent = NewObject<UAudioComponent>(Owner, TEXT("InstantBarkAudio"));
		if (AudioComponent)
		{
			AudioComponent->RegisterComponent();
			AudioComponent->AttachToComponent(Owner->GetRootComponent(),
			                                  FAttachmentTransformRules::KeepRelativeTransform);
			AudioComponent->bAutoActivate = false;
			AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
		}
	}

	// Setup default barks if library is empty
	if (BarkLibrary.GreetBarks.Num() == 0)
	{
		SetupDefaultBarks();
	}

	// Auto-bind to RFSN client
	if (bAutoBindToClient)
	{
		if (URfsnNpcClientComponent* Client = Owner->FindComponentByClass<URfsnNpcClientComponent>())
		{
			BindToRfsnClient(Client);
		}
	}

	RFSN_LOG(TEXT("InstantBark initialized for %s"), *GetOwner()->GetName());
}

void URfsnInstantBark::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopBark();
	Super::EndPlay(EndPlayReason);
}

void URfsnInstantBark::PlayBark(ERfsnBarkCategory Category)
{
	if (!bEnabled)
	{
		return;
	}

	StopBark();

	FRfsnInstantBarkEntry Bark = GetNextBark(Category);

	if (Bark.Text.IsEmpty())
	{
		return;
	}

	bIsPlaying = true;

	// Play audio if available
	if (Bark.Audio && AudioComponent)
	{
		AudioComponent->SetSound(Bark.Audio);
		AudioComponent->Play();
	}

	// Set timer for completion
	float Duration = Bark.DurationMs / 1000.0f;
	GetWorld()->GetTimerManager().SetTimer(BarkCompletionTimer, this, &URfsnInstantBark::OnAudioFinished, Duration,
	                                       false);

	// Broadcast event
	OnBarkPlayed.Broadcast(Category, Bark.Text);

	RFSN_LOG(TEXT("[InstantBark] Playing: '%s' (%dms)"), *Bark.Text, Bark.DurationMs);
}

void URfsnInstantBark::PlayBarkFromAction(const FString& ActionName)
{
	ERfsnBarkCategory Category = ActionToCategory(ActionName);
	PlayBark(Category);
}

void URfsnInstantBark::PlayServerBark(const FString& BarkText, int32 DurationMs)
{
	if (!bEnabled || BarkText.IsEmpty())
	{
		return;
	}

	StopBark();
	bIsPlaying = true;

	// Set timer for display duration
	float Duration = DurationMs / 1000.0f;
	GetWorld()->GetTimerManager().SetTimer(BarkCompletionTimer, this, &URfsnInstantBark::OnAudioFinished, Duration,
	                                       false);

	// Broadcast for subtitles
	OnBarkPlayed.Broadcast(ERfsnBarkCategory::Idle, BarkText);

	RFSN_LOG(TEXT("[InstantBark] Server bark: '%s' (%dms)"), *BarkText.Left(30), DurationMs);
}

ERfsnBarkCategory URfsnInstantBark::ActionToCategory(const FString& ActionName)
{
	FString Lower = ActionName.ToLower();

	if (Lower == TEXT("greet"))
		return ERfsnBarkCategory::Greet;
	if (Lower == TEXT("threaten"))
		return ERfsnBarkCategory::Threaten;
	if (Lower == TEXT("attack"))
		return ERfsnBarkCategory::Combat;
	if (Lower == TEXT("agree"))
		return ERfsnBarkCategory::Agree;
	if (Lower == TEXT("accept"))
		return ERfsnBarkCategory::Agree;
	if (Lower == TEXT("disagree"))
		return ERfsnBarkCategory::Disagree;
	if (Lower == TEXT("refuse"))
		return ERfsnBarkCategory::Disagree;
	if (Lower == TEXT("help"))
		return ERfsnBarkCategory::Help;
	if (Lower == TEXT("trade"))
		return ERfsnBarkCategory::Trade;
	if (Lower == TEXT("offer"))
		return ERfsnBarkCategory::Trade;
	if (Lower == TEXT("farewell"))
		return ERfsnBarkCategory::Farewell;
	if (Lower == TEXT("flee"))
		return ERfsnBarkCategory::Surprise;
	if (Lower == TEXT("apologize"))
		return ERfsnBarkCategory::Grateful;
	if (Lower == TEXT("talk"))
		return ERfsnBarkCategory::Idle;
	if (Lower == TEXT("question"))
		return ERfsnBarkCategory::Question;
	if (Lower == TEXT("answer"))
		return ERfsnBarkCategory::Question;

	return ERfsnBarkCategory::Idle;
}

TArray<FRfsnInstantBarkEntry> URfsnInstantBark::GetBarksForCategory(ERfsnBarkCategory Category) const
{
	switch (Category)
	{
	case ERfsnBarkCategory::Greet:
		return BarkLibrary.GreetBarks;
	case ERfsnBarkCategory::Threaten:
		return BarkLibrary.ThreatenBarks;
	case ERfsnBarkCategory::Agree:
		return BarkLibrary.AgreeBarks;
	case ERfsnBarkCategory::Disagree:
		return BarkLibrary.DisagreeBarks;
	case ERfsnBarkCategory::Question:
		return BarkLibrary.QuestionBarks;
	case ERfsnBarkCategory::Help:
		return BarkLibrary.HelpBarks;
	case ERfsnBarkCategory::Trade:
		return BarkLibrary.TradeBarks;
	case ERfsnBarkCategory::Farewell:
		return BarkLibrary.FarewellBarks;
	case ERfsnBarkCategory::Idle:
		return BarkLibrary.IdleBarks;
	case ERfsnBarkCategory::Combat:
		return BarkLibrary.CombatBarks;
	case ERfsnBarkCategory::Surprise:
		return BarkLibrary.SurpriseBarks;
	case ERfsnBarkCategory::Grateful:
		return BarkLibrary.GratefulBarks;
	default:
		return TArray<FRfsnInstantBarkEntry>();
	}
}

void URfsnInstantBark::StopBark()
{
	if (AudioComponent && AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
	}

	GetWorld()->GetTimerManager().ClearTimer(BarkCompletionTimer);
	bIsPlaying = false;
}

bool URfsnInstantBark::IsBarkPlaying() const
{
	return bIsPlaying;
}

void URfsnInstantBark::BindToRfsnClient(URfsnNpcClientComponent* Client)
{
	if (Client)
	{
		Client->OnMetaReceived.AddDynamic(this, &URfsnInstantBark::OnRfsnMetaReceived);
		RFSN_LOG(TEXT("InstantBark bound to RFSN client"));
	}
}

void URfsnInstantBark::SetupDefaultBarks()
{
	// Greet
	BarkLibrary.GreetBarks.Add(FRfsnInstantBarkEntry{TEXT("Hey there!"), nullptr, 400});
	BarkLibrary.GreetBarks.Add(FRfsnInstantBarkEntry{TEXT("Well, hello!"), nullptr, 450});
	BarkLibrary.GreetBarks.Add(FRfsnInstantBarkEntry{TEXT("Ah, you again."), nullptr, 500});

	// Threaten
	BarkLibrary.ThreatenBarks.Add(FRfsnInstantBarkEntry{TEXT("You asked for it!"), nullptr, 600});
	BarkLibrary.ThreatenBarks.Add(FRfsnInstantBarkEntry{TEXT("Don't test me."), nullptr, 500});
	BarkLibrary.ThreatenBarks.Add(FRfsnInstantBarkEntry{TEXT("I'm warning you."), nullptr, 550});

	// Agree
	BarkLibrary.AgreeBarks.Add(FRfsnInstantBarkEntry{TEXT("Alright then."), nullptr, 400});
	BarkLibrary.AgreeBarks.Add(FRfsnInstantBarkEntry{TEXT("Fair enough."), nullptr, 400});
	BarkLibrary.AgreeBarks.Add(FRfsnInstantBarkEntry{TEXT("You got it."), nullptr, 350});

	// Disagree
	BarkLibrary.DisagreeBarks.Add(FRfsnInstantBarkEntry{TEXT("I don't think so."), nullptr, 500});
	BarkLibrary.DisagreeBarks.Add(FRfsnInstantBarkEntry{TEXT("No way."), nullptr, 300});
	BarkLibrary.DisagreeBarks.Add(FRfsnInstantBarkEntry{TEXT("Not a chance."), nullptr, 450});

	// Question
	BarkLibrary.QuestionBarks.Add(FRfsnInstantBarkEntry{TEXT("Hmm, let me think..."), nullptr, 600});
	BarkLibrary.QuestionBarks.Add(FRfsnInstantBarkEntry{TEXT("Good question."), nullptr, 400});
	BarkLibrary.QuestionBarks.Add(FRfsnInstantBarkEntry{TEXT("Well..."), nullptr, 300});

	// Help
	BarkLibrary.HelpBarks.Add(FRfsnInstantBarkEntry{TEXT("Of course!"), nullptr, 350});
	BarkLibrary.HelpBarks.Add(FRfsnInstantBarkEntry{TEXT("I can help with that."), nullptr, 600});
	BarkLibrary.HelpBarks.Add(FRfsnInstantBarkEntry{TEXT("Let's see..."), nullptr, 400});

	// Trade
	BarkLibrary.TradeBarks.Add(FRfsnInstantBarkEntry{TEXT("Looking to trade?"), nullptr, 500});
	BarkLibrary.TradeBarks.Add(FRfsnInstantBarkEntry{TEXT("Let's see what you've got."), nullptr, 600});
	BarkLibrary.TradeBarks.Add(FRfsnInstantBarkEntry{TEXT("Business, eh?"), nullptr, 400});

	// Farewell
	BarkLibrary.FarewellBarks.Add(FRfsnInstantBarkEntry{TEXT("Take care."), nullptr, 350});
	BarkLibrary.FarewellBarks.Add(FRfsnInstantBarkEntry{TEXT("Until next time."), nullptr, 450});
	BarkLibrary.FarewellBarks.Add(FRfsnInstantBarkEntry{TEXT("Safe travels."), nullptr, 400});

	// Idle
	BarkLibrary.IdleBarks.Add(FRfsnInstantBarkEntry{TEXT("Hmm."), nullptr, 200});
	BarkLibrary.IdleBarks.Add(FRfsnInstantBarkEntry{TEXT("..."), nullptr, 100});

	// Combat
	BarkLibrary.CombatBarks.Add(FRfsnInstantBarkEntry{TEXT("Die!"), nullptr, 250});
	BarkLibrary.CombatBarks.Add(FRfsnInstantBarkEntry{TEXT("Take that!"), nullptr, 300});
	BarkLibrary.CombatBarks.Add(FRfsnInstantBarkEntry{TEXT("You'll regret this!"), nullptr, 500});

	// Surprise
	BarkLibrary.SurpriseBarks.Add(FRfsnInstantBarkEntry{TEXT("What the—"), nullptr, 350});
	BarkLibrary.SurpriseBarks.Add(FRfsnInstantBarkEntry{TEXT("Whoa!"), nullptr, 250});
	BarkLibrary.SurpriseBarks.Add(FRfsnInstantBarkEntry{TEXT("Huh?"), nullptr, 200});

	// Grateful
	BarkLibrary.GratefulBarks.Add(FRfsnInstantBarkEntry{TEXT("Thanks!"), nullptr, 300});
	BarkLibrary.GratefulBarks.Add(FRfsnInstantBarkEntry{TEXT("Much appreciated."), nullptr, 450});
	BarkLibrary.GratefulBarks.Add(FRfsnInstantBarkEntry{TEXT("You're too kind."), nullptr, 450});
}

void URfsnInstantBark::OnRfsnMetaReceived(const FRfsnDialogueMeta& Meta)
{
	// Play bark immediately when action is received
	// This masks the LLM generation latency
	FString ActionStr = UEnum::GetValueAsString(Meta.NpcAction);
	ActionStr = ActionStr.Replace(TEXT("ERfsnNpcAction::"), TEXT(""));

	PlayBarkFromAction(ActionStr);
}

FRfsnInstantBarkEntry URfsnInstantBark::GetNextBark(ERfsnBarkCategory Category)
{
	TArray<FRfsnInstantBarkEntry> Barks = GetBarksForCategory(Category);

	if (Barks.Num() == 0)
	{
		return FRfsnInstantBarkEntry{TEXT("..."), nullptr, 100};
	}

	// Round-robin selection
	int32& Index = BarkIndices.FindOrAdd(Category);
	FRfsnInstantBarkEntry Bark = Barks[Index % Barks.Num()];
	Index = (Index + 1) % Barks.Num();

	return Bark;
}

void URfsnInstantBark::OnAudioFinished()
{
	bIsPlaying = false;
	OnBarkComplete.Broadcast();
}
