// RFSN Reputation HUD Implementation

#include "RfsnReputationHud.h"
#include "RfsnFactionSystem.h"
#include "RfsnLogging.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

URfsnReputationHud::URfsnReputationHud()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f; // Update 2x per second
}

void URfsnReputationHud::BeginPlay()
{
	Super::BeginPlay();

	// Populate tracked factions if empty
	if (TrackedFactions.Num() == 0)
	{
		TrackedFactions.Add(TEXT("survivors"));
		TrackedFactions.Add(TEXT("bandits"));
		TrackedFactions.Add(TEXT("military"));
		TrackedFactions.Add(TEXT("merchants"));
		TrackedFactions.Add(TEXT("cultists"));
	}

	// Initial data refresh
	RefreshFactionData();

	RFSN_LOG(TEXT("ReputationHud initialized, tracking %d factions"), TrackedFactions.Num());
}

void URfsnReputationHud::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Auto-hide timer
	if (bIsVisible && AutoHideDelay > 0.0f && HideTimer > 0.0f)
	{
		HideTimer -= DeltaTime;
		if (HideTimer <= 0.0f)
		{
			HideHud();
		}
	}

	// Refresh data periodically
	RefreshFactionData();
}

void URfsnReputationHud::RefreshFactionData()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	URfsnFactionSystem* FactionSys = nullptr;
	if (UGameInstance* GI = World->GetGameInstance())
	{
		FactionSys = GI->GetSubsystem<URfsnFactionSystem>();
	}

	TArray<FRfsnFactionDisplay> NewData;
	bool bAnyChanged = false;

	for (const FString& FactionId : TrackedFactions)
	{
		FRfsnFactionDisplay Display;
		Display.FactionId = FactionId;
		Display.DisplayName = GetFactionDisplayName(FactionId);
		Display.IconSymbol = GetFactionIcon(FactionId);

		if (FactionSys)
		{
			Display.Reputation = FactionSys->GetReputation(FactionId);
			Display.TierText = FactionSys->GetReputationTier(FactionId);
		}
		else
		{
			Display.Reputation = 0.0f;
			Display.TierText = TEXT("Neutral");
		}

		Display.TierColor = GetTierColor(Display.Reputation);
		Display.NormalizedValue = (Display.Reputation + 100.0f) / 200.0f;

		// Check for significant change
		float* PrevRep = PreviousReputations.Find(FactionId);
		if (PrevRep)
		{
			float Delta = Display.Reputation - *PrevRep;
			if (FMath::Abs(Delta) >= NotificationThreshold)
			{
				bAnyChanged = true;
				OnReputationChanged.Broadcast(FactionId, Display.Reputation);

				if (bShowChangeNotifications)
				{
					RFSN_LOG(TEXT("Reputation with %s changed: %.1f -> %.1f"), *Display.DisplayName, *PrevRep,
					         Display.Reputation);
				}
			}
		}

		PreviousReputations.Add(FactionId, Display.Reputation);
		NewData.Add(Display);
	}

	CachedFactionData = NewData;

	if (bAnyChanged)
	{
		OnReputationDisplayUpdated.Broadcast(CachedFactionData);

		// Auto-show on change
		if (bShowChangeNotifications && !bIsVisible)
		{
			ShowHud();
		}
	}
}

FRfsnFactionDisplay URfsnReputationHud::GetFactionDisplayData(const FString& FactionId) const
{
	for (const FRfsnFactionDisplay& Display : CachedFactionData)
	{
		if (Display.FactionId.Equals(FactionId, ESearchCase::IgnoreCase))
		{
			return Display;
		}
	}

	// Return default
	FRfsnFactionDisplay Default;
	Default.FactionId = FactionId;
	Default.DisplayName = FactionId;
	Default.TierText = TEXT("Unknown");
	return Default;
}

void URfsnReputationHud::ShowHud()
{
	bIsVisible = true;
	HideTimer = AutoHideDelay;
	RefreshFactionData();
	OnReputationDisplayUpdated.Broadcast(CachedFactionData);
}

void URfsnReputationHud::HideHud()
{
	bIsVisible = false;
	HideTimer = 0.0f;
}

void URfsnReputationHud::ToggleHud()
{
	if (bIsVisible)
	{
		HideHud();
	}
	else
	{
		ShowHud();
	}
}

FLinearColor URfsnReputationHud::GetTierColor(float Reputation)
{
	// Hostile: Red, Unfriendly: Orange, Neutral: Gray, Friendly: Green, Allied: Blue
	if (Reputation <= -60.0f)
	{
		return FLinearColor(0.8f, 0.1f, 0.1f); // Hostile - Dark Red
	}
	if (Reputation <= -20.0f)
	{
		return FLinearColor(0.9f, 0.5f, 0.2f); // Unfriendly - Orange
	}
	if (Reputation <= 20.0f)
	{
		return FLinearColor(0.6f, 0.6f, 0.6f); // Neutral - Gray
	}
	if (Reputation <= 60.0f)
	{
		return FLinearColor(0.3f, 0.7f, 0.3f); // Friendly - Green
	}
	return FLinearColor(0.2f, 0.5f, 0.9f); // Allied - Blue
}

FString URfsnReputationHud::GetTierText(float Reputation)
{
	if (Reputation <= -60.0f)
	{
		return TEXT("Hostile");
	}
	if (Reputation <= -20.0f)
	{
		return TEXT("Unfriendly");
	}
	if (Reputation <= 20.0f)
	{
		return TEXT("Neutral");
	}
	if (Reputation <= 60.0f)
	{
		return TEXT("Friendly");
	}
	return TEXT("Allied");
}

FString URfsnReputationHud::GetFactionIcon(const FString& FactionId)
{
	FString Lower = FactionId.ToLower();

	if (Lower == TEXT("survivors"))
	{
		return TEXT("🏠"); // House
	}
	if (Lower == TEXT("bandits"))
	{
		return TEXT("💀"); // Skull
	}
	if (Lower == TEXT("military"))
	{
		return TEXT("🎖️"); // Medal
	}
	if (Lower == TEXT("merchants"))
	{
		return TEXT("💰"); // Money bag
	}
	if (Lower == TEXT("cultists"))
	{
		return TEXT("👁️"); // Eye
	}

	return TEXT("⚪"); // Generic circle
}

FString URfsnReputationHud::GetFactionDisplayName(const FString& FactionId)
{
	FString Lower = FactionId.ToLower();

	if (Lower == TEXT("survivors"))
	{
		return TEXT("Survivors");
	}
	if (Lower == TEXT("bandits"))
	{
		return TEXT("Bandits");
	}
	if (Lower == TEXT("military"))
	{
		return TEXT("Military");
	}
	if (Lower == TEXT("merchants"))
	{
		return TEXT("Merchants Guild");
	}
	if (Lower == TEXT("cultists"))
	{
		return TEXT("The Cult");
	}

	// Capitalize first letter
	FString Result = FactionId;
	if (Result.Len() > 0)
	{
		Result[0] = FChar::ToUpper(Result[0]);
	}
	return Result;
}
