// RFSN Weather Reactions System
// NPCs react to and comment on weather and environmental conditions
// Affects NPC behavior, dialogue context, and schedules

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RfsnWeatherReactions.generated.h"

/**
 * Current weather type
 */
UENUM(BlueprintType)
enum class ERfsnWeatherType : uint8
{
	Clear UMETA(DisplayName = "Clear"),
	Cloudy UMETA(DisplayName = "Cloudy"),
	Rain UMETA(DisplayName = "Rain"),
	Storm UMETA(DisplayName = "Storm"),
	Fog UMETA(DisplayName = "Fog"),
	Snow UMETA(DisplayName = "Snow"),
	Windy UMETA(DisplayName = "Windy"),
	Hot UMETA(DisplayName = "Hot"),
	Cold UMETA(DisplayName = "Cold")
};

/**
 * Time of day period
 */
UENUM(BlueprintType)
enum class ERfsnTimeOfDay : uint8
{
	Dawn UMETA(DisplayName = "Dawn"),           // 5-7
	Morning UMETA(DisplayName = "Morning"),     // 7-12
	Noon UMETA(DisplayName = "Noon"),           // 12-14
	Afternoon UMETA(DisplayName = "Afternoon"), // 14-18
	Evening UMETA(DisplayName = "Evening"),     // 18-21
	Night UMETA(DisplayName = "Night"),         // 21-5
	Midnight UMETA(DisplayName = "Midnight")    // 0-3
};

/**
 * Weather reaction type for NPC
 */
UENUM(BlueprintType)
enum class ERfsnWeatherReaction : uint8
{
	Neutral UMETA(DisplayName = "Neutral"),             // No special reaction
	SeekShelter UMETA(DisplayName = "Seek Shelter"),    // Go indoors
	Uncomfortable UMETA(DisplayName = "Uncomfortable"), // Complain
	Enjoying UMETA(DisplayName = "Enjoying"),           // Happy about it
	Worried UMETA(DisplayName = "Worried"),             // Concerned
	WorkFaster UMETA(DisplayName = "Work Faster")       // Hurry up
};

/**
 * Weather preference for this NPC
 */
USTRUCT(BlueprintType)
struct FRfsnWeatherPreference
{
	GENERATED_BODY()

	/** Weather type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	ERfsnWeatherType Weather = ERfsnWeatherType::Clear;

	/** Preference from -1 (hates) to 1 (loves) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "-1", ClampMax = "1"))
	float Preference = 0.0f;

	/** Custom comment for this weather */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FString Comment;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeatherChanged, ERfsnWeatherType, NewWeather, ERfsnWeatherType,
                                             OldWeather);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeOfDayChanged, ERfsnTimeOfDay, NewTime, ERfsnTimeOfDay, OldTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeatherReaction, ERfsnWeatherReaction, Reaction);

/**
 * Weather Reactions Component
 * NPCs react to environmental conditions
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API URfsnWeatherReactions : public UActorComponent
{
	GENERATED_BODY()

public:
	URfsnWeatherReactions();

	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Weather preferences for this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Config")
	TArray<FRfsnWeatherPreference> Preferences;

	/** Does this NPC seek shelter in bad weather? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Config")
	bool bSeeksShelter = true;

	/** Is this NPC currently indoors? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Config")
	bool bIsIndoors = false;

	/** Shelter location (if seeking shelter) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Config")
	FVector ShelterLocation = FVector::ZeroVector;

	/** Threshold for weather comments (-1 to 1, triggers if preference exceeds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather|Config")
	float CommentThreshold = 0.3f;

	// ─────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────

	/** Current weather */
	UPROPERTY(BlueprintReadOnly, Category = "Weather|State")
	ERfsnWeatherType CurrentWeather = ERfsnWeatherType::Clear;

	/** Current time of day */
	UPROPERTY(BlueprintReadOnly, Category = "Weather|State")
	ERfsnTimeOfDay CurrentTimeOfDay = ERfsnTimeOfDay::Morning;

	/** Current game hour (0-24) */
	UPROPERTY(BlueprintReadOnly, Category = "Weather|State")
	float CurrentHour = 12.0f;

	/** Current reaction */
	UPROPERTY(BlueprintReadOnly, Category = "Weather|State")
	ERfsnWeatherReaction CurrentReaction = ERfsnWeatherReaction::Neutral;

	/** Should NPC be seeking shelter? */
	UPROPERTY(BlueprintReadOnly, Category = "Weather|State")
	bool bShouldSeekShelter = false;

	// ─────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Weather|Events")
	FOnWeatherChanged OnWeatherChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weather|Events")
	FOnTimeOfDayChanged OnTimeOfDayChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weather|Events")
	FOnWeatherReaction OnWeatherReaction;

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Set current weather (usually called by game weather system) */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetWeather(ERfsnWeatherType NewWeather);

	/** Set current game time */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetGameTime(float Hour);

	/** Get NPC's feeling about current weather */
	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetWeatherFeeling() const;

	/** Get a weather-related comment */
	UFUNCTION(BlueprintPure, Category = "Weather")
	FString GetWeatherComment() const;

	/** Get time-of-day greeting */
	UFUNCTION(BlueprintPure, Category = "Weather")
	FString GetTimeGreeting() const;

	/** Get full environment context for LLM */
	UFUNCTION(BlueprintPure, Category = "Weather")
	FString GetEnvironmentContext() const;

	/** Get behavior modifier from weather */
	UFUNCTION(BlueprintPure, Category = "Weather")
	float GetWeatherBehaviorModifier() const;

	/** Check if NPC should go indoors */
	UFUNCTION(BlueprintPure, Category = "Weather")
	bool ShouldSeekShelter() const;

	/** Weather type to string */
	UFUNCTION(BlueprintPure, Category = "Weather")
	static FString WeatherToString(ERfsnWeatherType Weather);

	/** Time of day to string */
	UFUNCTION(BlueprintPure, Category = "Weather")
	static FString TimeOfDayToString(ERfsnTimeOfDay Time);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Update time of day from hour */
	void UpdateTimeOfDay();

	/** Calculate current reaction */
	void UpdateReaction();

	/** Get preference for weather type */
	float GetPreference(ERfsnWeatherType Weather) const;

	/** Get custom comment for weather */
	FString GetCustomComment(ERfsnWeatherType Weather) const;
};
