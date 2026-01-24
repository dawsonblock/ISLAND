// RFSN HTTP Connection Pool Implementation

#include "RfsnHttpPool.h"
#include "RfsnLogging.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"

void URfsnHttpPool::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetStats();
	PingServer();
	RFSN_LOG("HTTP Pool initialized - BaseUrl: %s", *BaseUrl);
}

TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> URfsnHttpPool::CreatePostRequest(const FString& Endpoint,
                                                                               const FString& JsonBody)
{
	if (Stats.ActiveRequests >= MaxConcurrentRequests)
	{
		RFSN_WARNING("Max concurrent requests reached (%d)", MaxConcurrentRequests);
		// Could queue request here
	}

	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	FString FullUrl = BaseUrl + Endpoint;
	Request->SetURL(FullUrl);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json, text/event-stream"));
	Request->SetContentAsString(JsonBody);
	Request->SetTimeout(RequestTimeout);

	return Request;
}

TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> URfsnHttpPool::CreateGetRequest(const FString& Endpoint)
{
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	FString FullUrl = BaseUrl + Endpoint;
	Request->SetURL(FullUrl);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetTimeout(RequestTimeout);

	return Request;
}

void URfsnHttpPool::ResetStats()
{
	Stats = FRfsnHttpStats();
	LatencySamples.Empty();
}

void URfsnHttpPool::PingServer()
{
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = CreateGetRequest(TEXT("/api/health"));

	Request->OnProcessRequestComplete().BindLambda(
	    [this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
	    {
		    bServerAvailable = bSuccess && Response.IsValid() && Response->GetResponseCode() == 200;

		    if (bServerAvailable)
		    {
			    RFSN_LOG("RFSN server is available");
		    }
		    else
		    {
			    RFSN_WARNING("RFSN server is not available");
		    }
	    });

	Request->ProcessRequest();
}

void URfsnHttpPool::OnRequestStarted()
{
	Stats.TotalRequests++;
	Stats.ActiveRequests++;
}

void URfsnHttpPool::OnRequestCompleted(bool bSuccess, float LatencyMs, int32 BytesReceived)
{
	Stats.ActiveRequests = FMath::Max(0, Stats.ActiveRequests - 1);

	if (bSuccess)
	{
		Stats.SuccessCount++;
	}
	else
	{
		Stats.ErrorCount++;
	}

	Stats.TotalDataReceivedKb += BytesReceived / 1024.0f;
	UpdateAverageLatency(LatencyMs);
}

void URfsnHttpPool::UpdateAverageLatency(float NewSample)
{
	const int32 MaxSamples = 100;

	LatencySamples.Add(NewSample);
	if (LatencySamples.Num() > MaxSamples)
	{
		LatencySamples.RemoveAt(0);
	}

	float Sum = 0.0f;
	for (float Sample : LatencySamples)
	{
		Sum += Sample;
	}
	Stats.AverageLatencyMs = Sum / LatencySamples.Num();
}
