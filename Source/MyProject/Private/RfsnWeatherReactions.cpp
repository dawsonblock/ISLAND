// RFSN Weather Reactions Implementation

#include "RfsnWeatherReactions.h"
#include "RfsnLogging.h"
#include "RfsnEmotionBlend.h"

URfsnWeatherReactions::URfsnWeatherReactions()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 5.0f; // Update every 5 seconds
}

void URfsnWeatherReactions::BeginPlay()
{
	Super::BeginPlay();

	// Setup default preferences if empty
	if (Preferences.Num() == 0)
	{
		Preferences.Add({ERfsnWeatherType::Rain, -0.3f, TEXT("I hope this rain stops soon.")});
		Preferences.Add({ERfsnWeatherType::Storm, -0.8f, TEXT("This storm is dangerous!")});
		Preferences.Add({ERfsnWeatherType::Clear, 0.5f, TEXT("Beautiful day, isn't it?")});
		Preferences.Add({ERfsnWeatherType::Cold, -0.4f, TEXT("It's freezing out here.")});
	}

	RFSN_LOG(TEXT("WeatherReactions initialized for %s"), *GetOwner()->GetName());
}

void URfsnWeatherReactions::TickComponent(float DeltaTime, ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateTimeOfDay();
	UpdateReaction();
}

void URfsnWeatherReactions::SetWeather(ERfsnWeatherType NewWeather)
{
	if (NewWeather != CurrentWeather)
	{
		ERfsnWeatherType OldWeather = CurrentWeather;
		CurrentWeather = NewWeather;
		OnWeatherChanged.Broadcast(NewWeather, OldWeather);
		UpdateReaction();

		RFSN_LOG(TEXT("%s noticed weather change: %s -> %s"), *GetOwner()->GetName(), *WeatherToString(OldWeather),
		         *WeatherToString(NewWeather));
	}
}

void URfsnWeatherReactions::SetGameTime(float Hour)
{
	CurrentHour = FMath::Fmod(Hour, 24.0f);
	if (CurrentHour < 0)
		CurrentHour += 24.0f;
	UpdateTimeOfDay();
}

void URfsnWeatherReactions::UpdateTimeOfDay()
{
	ERfsnTimeOfDay NewTime;

	if (CurrentHour >= 0 && CurrentHour < 3)
	{
		NewTime = ERfsnTimeOfDay::Midnight;
	}
	else if (CurrentHour >= 3 && CurrentHour < 5)
	{
		NewTime = ERfsnTimeOfDay::Night;
	}
	else if (CurrentHour >= 5 && CurrentHour < 7)
	{
		NewTime = ERfsnTimeOfDay::Dawn;
	}
	else if (CurrentHour >= 7 && CurrentHour < 12)
	{
		NewTime = ERfsnTimeOfDay::Morning;
	}
	else if (CurrentHour >= 12 && CurrentHour < 14)
	{
		NewTime = ERfsnTimeOfDay::Noon;
	}
	else if (CurrentHour >= 14 && CurrentHour < 18)
	{
		NewTime = ERfsnTimeOfDay::Afternoon;
	}
	else if (CurrentHour >= 18 && CurrentHour < 21)
	{
		NewTime = ERfsnTimeOfDay::Evening;
	}
	else
	{
		NewTime = ERfsnTimeOfDay::Night;
	}

	if (NewTime != CurrentTimeOfDay)
	{
		ERfsnTimeOfDay OldTime = CurrentTimeOfDay;
		CurrentTimeOfDay = NewTime;
		OnTimeOfDayChanged.Broadcast(NewTime, OldTime);
	}
}

void URfsnWeatherReactions::UpdateReaction()
{
	float Feeling = GetWeatherFeeling();
	ERfsnWeatherReaction NewReaction;

	// Determine reaction based on feeling and weather severity
	if (CurrentWeather == ERfsnWeatherType::Storm && !bIsIndoors)
	{
		NewReaction = ERfsnWeatherReaction::SeekShelter;
		bShouldSeekShelter = true;
	}
	else if (CurrentWeather == ERfsnWeatherType::Rain && !bIsIndoors && bSeeksShelter)
	{
		NewReaction = ERfsnWeatherReaction::SeekShelter;
		bShouldSeekShelter = true;
	}
	else if (Feeling < -0.5f)
	{
		NewReaction = ERfsnWeatherReaction::Uncomfortable;
		bShouldSeekShelter = bSeeksShelter && !bIsIndoors;
	}
	else if (Feeling < -0.2f)
	{
		NewReaction = ERfsnWeatherReaction::Worried;
		bShouldSeekShelter = false;
	}
	else if (Feeling > 0.3f)
	{
		NewReaction = ERfsnWeatherReaction::Enjoying;
		bShouldSeekShelter = false;
	}
	else
	{
		NewReaction = ERfsnWeatherReaction::Neutral;
		bShouldSeekShelter = false;
	}

	if (NewReaction != CurrentReaction)
	{
		CurrentReaction = NewReaction;
		OnWeatherReaction.Broadcast(NewReaction);
	}
}

float URfsnWeatherReactions::GetWeatherFeeling() const
{
	return GetPreference(CurrentWeather);
}

float URfsnWeatherReactions::GetPreference(ERfsnWeatherType Weather) const
{
	for (const FRfsnWeatherPreference& Pref : Preferences)
	{
		if (Pref.Weather == Weather)
		{
			return Pref.Preference;
		}
	}
	return 0.0f; // Neutral
}

FString URfsnWeatherReactions::GetCustomComment(ERfsnWeatherType Weather) const
{
	for (const FRfsnWeatherPreference& Pref : Preferences)
	{
		if (Pref.Weather == Weather && !Pref.Comment.IsEmpty())
		{
			return Pref.Comment;
		}
	}
	return TEXT("");
}

FString URfsnWeatherReactions::GetWeatherComment() const
{
	// Check for custom comment first
	FString Custom = GetCustomComment(CurrentWeather);
	if (!Custom.IsEmpty())
	{
		return Custom;
	}

	// Generate generic comment
	switch (CurrentWeather)
	{
	case ERfsnWeatherType::Clear:
		return TEXT("Nice weather we're having.");
	case ERfsnWeatherType::Cloudy:
		return TEXT("Looks like it might rain.");
	case ERfsnWeatherType::Rain:
		return bIsIndoors ? TEXT("Glad to be inside with this rain.") : TEXT("Getting wet out here.");
	case ERfsnWeatherType::Storm:
		return bIsIndoors ? TEXT("Quite a storm out there!") : TEXT("We need to find shelter!");
	case ERfsnWeatherType::Fog:
		return TEXT("Can barely see anything in this fog.");
	case ERfsnWeatherType::Snow:
		return TEXT("It's really coming down.");
	case ERfsnWeatherType::Windy:
		return TEXT("This wind is relentless.");
	case ERfsnWeatherType::Hot:
		return TEXT("It's so hot today.");
	case ERfsnWeatherType::Cold:
		return TEXT("It's freezing out here.");
	default:
		return TEXT("");
	}
}

FString URfsnWeatherReactions::GetTimeGreeting() const
{
	switch (CurrentTimeOfDay)
	{
	case ERfsnTimeOfDay::Dawn:
		return TEXT("You're up early.");
	case ERfsnTimeOfDay::Morning:
		return TEXT("Good morning.");
	case ERfsnTimeOfDay::Noon:
		return TEXT("Good day.");
	case ERfsnTimeOfDay::Afternoon:
		return TEXT("Good afternoon.");
	case ERfsnTimeOfDay::Evening:
		return TEXT("Good evening.");
	case ERfsnTimeOfDay::Night:
	case ERfsnTimeOfDay::Midnight:
		return TEXT("What brings you out at this hour?");
	default:
		return TEXT("Hello.");
	}
}

FString URfsnWeatherReactions::GetEnvironmentContext() const
{
	FString Context;

	// Time context
	Context += FString::Printf(TEXT("It is currently %s (around %d:00). "), *TimeOfDayToString(CurrentTimeOfDay),
	                           FMath::FloorToInt(CurrentHour));

	// Weather context
	if (CurrentWeather != ERfsnWeatherType::Clear)
	{
		Context += FString::Printf(TEXT("The weather is %s. "), *WeatherToString(CurrentWeather));
	}

	// Location context
	if (bIsIndoors)
	{
		Context += TEXT("You are indoors. ");
	}
	else
	{
		Context += TEXT("You are outside. ");
	}

	// Reaction context
	float Feeling = GetWeatherFeeling();
	if (FMath::Abs(Feeling) > CommentThreshold)
	{
		if (Feeling > 0)
		{
			Context += TEXT("NPC is enjoying the conditions. ");
		}
		else
		{
			Context += TEXT("NPC is uncomfortable with the conditions. ");
		}
	}

	return Context;
}

float URfsnWeatherReactions::GetWeatherBehaviorModifier() const
{
	// Used to modify NPC speed, alertness, etc.
	float Modifier = 0.0f;

	switch (CurrentWeather)
	{
	case ERfsnWeatherType::Storm:
		Modifier = -0.3f; // Slower, more cautious
		break;
	case ERfsnWeatherType::Rain:
		Modifier = -0.15f;
		break;
	case ERfsnWeatherType::Fog:
		Modifier = -0.2f; // Reduced visibility
		break;
	case ERfsnWeatherType::Cold:
	case ERfsnWeatherType::Hot:
		Modifier = -0.1f;
		break;
	case ERfsnWeatherType::Clear:
		Modifier = 0.1f; // Bonus
		break;
	default:
		break;
	}

	// Night penalty
	if (CurrentTimeOfDay == ERfsnTimeOfDay::Night || CurrentTimeOfDay == ERfsnTimeOfDay::Midnight)
	{
		Modifier -= 0.2f;
	}

	return FMath::Clamp(Modifier, -0.5f, 0.5f);
}

bool URfsnWeatherReactions::ShouldSeekShelter() const
{
	return bShouldSeekShelter && !bIsIndoors;
}

FString URfsnWeatherReactions::WeatherToString(ERfsnWeatherType Weather)
{
	switch (Weather)
	{
	case ERfsnWeatherType::Clear:
		return TEXT("clear");
	case ERfsnWeatherType::Cloudy:
		return TEXT("cloudy");
	case ERfsnWeatherType::Rain:
		return TEXT("rainy");
	case ERfsnWeatherType::Storm:
		return TEXT("stormy");
	case ERfsnWeatherType::Fog:
		return TEXT("foggy");
	case ERfsnWeatherType::Snow:
		return TEXT("snowy");
	case ERfsnWeatherType::Windy:
		return TEXT("windy");
	case ERfsnWeatherType::Hot:
		return TEXT("hot");
	case ERfsnWeatherType::Cold:
		return TEXT("cold");
	default:
		return TEXT("unknown");
	}
}

FString URfsnWeatherReactions::TimeOfDayToString(ERfsnTimeOfDay Time)
{
	switch (Time)
	{
	case ERfsnTimeOfDay::Dawn:
		return TEXT("dawn");
	case ERfsnTimeOfDay::Morning:
		return TEXT("morning");
	case ERfsnTimeOfDay::Noon:
		return TEXT("midday");
	case ERfsnTimeOfDay::Afternoon:
		return TEXT("afternoon");
	case ERfsnTimeOfDay::Evening:
		return TEXT("evening");
	case ERfsnTimeOfDay::Night:
		return TEXT("night");
	case ERfsnTimeOfDay::Midnight:
		return TEXT("midnight");
	default:
		return TEXT("day");
	}
}
