// RFSN HTTP Connection Pool
// Manages pooled HTTP connections for improved performance

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "RfsnHttpPool.generated.h"

USTRUCT(BlueprintType)
struct FRfsnHttpStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalRequests = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ActiveRequests = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 SuccessCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ErrorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float AverageLatencyMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float TotalDataReceivedKb = 0.0f;
};

/**
 * Game Instance Subsystem for managing HTTP connections.
 * Provides request pooling, retry logic, and statistics.
 */
UCLASS()
class MYPROJECT_API URfsnHttpPool : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────
	// Configuration
	// ─────────────────────────────────────────────────────────────

	/** Maximum concurrent requests */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HTTP Pool")
	int32 MaxConcurrentRequests = 4;

	/** Request timeout in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HTTP Pool")
	float RequestTimeout = 30.0f;

	/** Number of retry attempts on failure */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HTTP Pool")
	int32 MaxRetries = 2;

	/** Base URL for RFSN server */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HTTP Pool")
	FString BaseUrl = TEXT("http://127.0.0.1:8000");

	// ─────────────────────────────────────────────────────────────
	// API
	// ─────────────────────────────────────────────────────────────

	/** Create a pooled POST request (C++ only - TSharedPtr not Blueprint-exposable) */
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> CreatePostRequest(const FString& Endpoint, const FString& JsonBody);

	/** Create a pooled GET request (C++ only - TSharedPtr not Blueprint-exposable) */
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> CreateGetRequest(const FString& Endpoint);

	/** Get current statistics */
	UFUNCTION(BlueprintPure, Category = "HTTP Pool")
	FRfsnHttpStats GetStats() const { return Stats; }

	/** Reset statistics */
	UFUNCTION(BlueprintCallable, Category = "HTTP Pool")
	void ResetStats();

	/** Check if server is reachable */
	UFUNCTION(BlueprintCallable, Category = "HTTP Pool")
	void PingServer();

	/** Is server available */
	UFUNCTION(BlueprintPure, Category = "HTTP Pool")
	bool IsServerAvailable() const { return bServerAvailable; }

	// Internal tracking
	void OnRequestStarted();
	void OnRequestCompleted(bool bSuccess, float LatencyMs, int32 BytesReceived);

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	FRfsnHttpStats Stats;
	bool bServerAvailable = false;
	TArray<float> LatencySamples;

	void UpdateAverageLatency(float NewSample);
};
