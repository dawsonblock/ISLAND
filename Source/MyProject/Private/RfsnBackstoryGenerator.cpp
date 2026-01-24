// RFSN Procedural Backstory Generator Implementation

#include "RfsnBackstoryGenerator.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RfsnFactionSystem.h"
#include "RfsnLogging.h"
#include "RfsnNpcClientComponent.h"
#include "RfsnTemporalMemory.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

URfsnBackstoryGenerator::URfsnBackstoryGenerator()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URfsnBackstoryGenerator::BeginPlay()
{
	Super::BeginPlay();

	// Find sibling RFSN client
	RfsnClient = GetOwner()->FindComponentByClass<URfsnNpcClientComponent>();

	// Load saved backstory if enabled
	if (bLoadOnBeginPlay)
	{
		LoadBackstory();
	}

	RFSN_LOG(TEXT("BackstoryGenerator initialized for %s (HasBackstory: %s)"), *GetOwner()->GetName(),
	         HasBackstory() ? TEXT("Yes") : TEXT("No"));
}

void URfsnBackstoryGenerator::OnFirstInteraction()
{
	if (bHasInteracted)
	{
		return;
	}

	bHasInteracted = true;

	// Generate backstory if we don't have one
	if (!HasBackstory() && !bIsGenerating)
	{
		GenerateBackstory();
	}
}

void URfsnBackstoryGenerator::GenerateBackstory()
{
	if (bIsGenerating)
	{
		RFSN_LOG(TEXT("Backstory generation already in progress"));
		return;
	}

	bIsGenerating = true;

	// Build request
	FRfsnBackstoryRequest Request;
	Request.NpcId = RfsnClient ? RfsnClient->NpcId : GetOwner()->GetName();
	Request.NpcName = RfsnClient ? RfsnClient->NpcName : TEXT("Unknown");
	Request.Hint = BackstoryHint;
	Request.PersonalityTraits = PersonalityTraits;
	Request.Occupation = DefaultOccupation;
	Request.CurrentMood = RfsnClient ? RfsnClient->Mood : TEXT("Neutral");

	// Get faction info
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI)
	{
		if (URfsnFactionSystem* FactionSystem = GI->GetSubsystem<URfsnFactionSystem>())
		{
			// Try to find NPC's faction from client
			// For now, use a default based on location or type
			Request.FactionId = TEXT("survivors");
		}
	}

	// Build JSON
	FString JsonPayload = BuildRequestJson(Request);

	// Send HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(BackstoryEndpoint);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(JsonPayload);

	HttpRequest->OnProcessRequestComplete().BindLambda(
	    [this](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
	    {
		    FString Response = bSuccess && Res.IsValid() ? Res->GetContentAsString() : TEXT("");

		    // Must run on game thread
		    AsyncTask(ENamedThreads::GameThread,
		              [this, bSuccess, Response]() { OnBackstoryRequestComplete(bSuccess, Response); });
	    });

	HttpRequest->ProcessRequest();

	RFSN_LOG(TEXT("Sending backstory generation request for %s"), *Request.NpcId);
}

FString URfsnBackstoryGenerator::BuildRequestJson(const FRfsnBackstoryRequest& Request) const
{
	TSharedRef<FJsonObject> JsonObj = MakeShared<FJsonObject>();

	JsonObj->SetStringField(TEXT("npc_id"), Request.NpcId);
	JsonObj->SetStringField(TEXT("npc_name"), Request.NpcName);
	JsonObj->SetStringField(TEXT("faction_id"), Request.FactionId);
	JsonObj->SetStringField(TEXT("hint"), Request.Hint);
	JsonObj->SetStringField(TEXT("occupation"), Request.Occupation);
	JsonObj->SetStringField(TEXT("current_mood"), Request.CurrentMood);
	JsonObj->SetNumberField(TEXT("summary_paragraphs"), 2); // User specified 2 paragraphs

	// Traits array
	TArray<TSharedPtr<FJsonValue>> TraitsArray;
	for (const FString& Trait : Request.PersonalityTraits)
	{
		TraitsArray.Add(MakeShared<FJsonValueString>(Trait));
	}
	JsonObj->SetArrayField(TEXT("personality_traits"), TraitsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObj, Writer);

	return OutputString;
}

void URfsnBackstoryGenerator::OnBackstoryRequestComplete(bool bSuccess, const FString& Response)
{
	bIsGenerating = false;

	if (!bSuccess || Response.IsEmpty())
	{
		RFSN_ERROR(TEXT("Backstory generation failed, using fallback"));
		CachedBackstory = GenerateFallbackBackstory();
		OnBackstoryError.Broadcast(TEXT("Connection failed"));
	}
	else
	{
		if (!ParseBackstoryResponse(Response, CachedBackstory))
		{
			RFSN_ERROR(TEXT("Failed to parse backstory response, using fallback"));
			CachedBackstory = GenerateFallbackBackstory();
			OnBackstoryError.Broadcast(TEXT("Parse failed"));
		}
	}

	CachedBackstory.GeneratedAt = FDateTime::Now();

	// Save if enabled
	if (bSaveAfterGeneration)
	{
		SaveBackstory();
	}

	// Notify listeners
	OnBackstoryGenerated.Broadcast(CachedBackstory);

	RFSN_LOG(TEXT("Backstory generated for %s: %s"), *CachedBackstory.NpcId, *CachedBackstory.Summary.Left(100));
}

bool URfsnBackstoryGenerator::ParseBackstoryResponse(const FString& JsonResponse, FRfsnNpcBackstory& OutBackstory)
{
	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonResponse);

	if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
	{
		return false;
	}

	OutBackstory.NpcId = JsonObj->GetStringField(TEXT("npc_id"));
	OutBackstory.Summary = JsonObj->GetStringField(TEXT("summary"));
	OutBackstory.Occupation = JsonObj->GetStringField(TEXT("occupation"));
	OutBackstory.FactionHistory = JsonObj->GetStringField(TEXT("faction_history"));
	OutBackstory.PersonalGoal = JsonObj->GetStringField(TEXT("personal_goal"));
	OutBackstory.Fear = JsonObj->GetStringField(TEXT("fear"));
	OutBackstory.SecretOrShame = JsonObj->GetStringField(TEXT("secret"));
	OutBackstory.DistinguishingTrait = JsonObj->GetStringField(TEXT("trait"));
	OutBackstory.GenerationVersion = JsonObj->GetIntegerField(TEXT("version"));

	// Parse elements array
	const TArray<TSharedPtr<FJsonValue>>* ElementsArray;
	if (JsonObj->TryGetArrayField(TEXT("elements"), ElementsArray))
	{
		for (const TSharedPtr<FJsonValue>& ElementVal : *ElementsArray)
		{
			const TSharedPtr<FJsonObject>* ElementObj;
			if (ElementVal->TryGetObject(ElementObj))
			{
				FRfsnBackstoryElement Element;
				Element.ElementType = (*ElementObj)->GetStringField(TEXT("type"));
				Element.Description = (*ElementObj)->GetStringField(TEXT("description"));
				Element.Importance = (*ElementObj)->GetNumberField(TEXT("importance"));
				Element.bPublicKnowledge = (*ElementObj)->GetBoolField(TEXT("public"));

				const TArray<TSharedPtr<FJsonValue>>* TagsArray;
				if ((*ElementObj)->TryGetArrayField(TEXT("tags"), TagsArray))
				{
					for (const TSharedPtr<FJsonValue>& TagVal : *TagsArray)
					{
						Element.Tags.Add(TagVal->AsString());
					}
				}

				OutBackstory.Elements.Add(Element);
			}
		}
	}

	return !OutBackstory.Summary.IsEmpty();
}

FRfsnNpcBackstory URfsnBackstoryGenerator::GenerateFallbackBackstory() const
{
	FRfsnNpcBackstory Fallback;

	FString Name = RfsnClient ? RfsnClient->NpcName : TEXT("This person");
	FString NpcId = RfsnClient ? RfsnClient->NpcId : GetOwner()->GetName();

	Fallback.NpcId = NpcId;
	Fallback.Occupation = DefaultOccupation.IsEmpty() ? TEXT("Survivor") : DefaultOccupation;

	// Generate two paragraphs based on traits
	FString Paragraph1 = FString::Printf(
	    TEXT("%s has lived on the island for as long as anyone can remember. "
	         "Before the collapse, they led a quiet life, but circumstances forced them to adapt quickly. "
	         "Now they work as a %s, doing what they can to survive."),
	    *Name, *Fallback.Occupation);

	FString TraitStr = PersonalityTraits.Num() > 0 ? PersonalityTraits[0] : TEXT("cautious");
	FString Paragraph2 =
	    FString::Printf(TEXT("Known for being %s, %s doesn't easily trust newcomers. "
	                         "They've seen too many people come and go, and have learned to rely on themselves first. "
	                         "Still, those who earn their respect find a loyal ally."),
	                    *TraitStr, *Name);

	Fallback.Summary = Paragraph1 + TEXT("\n\n") + Paragraph2;
	Fallback.PersonalGoal = TEXT("To find safety and stability");
	Fallback.Fear = TEXT("Being alone when it matters most");
	Fallback.SecretOrShame = TEXT("Once abandoned someone who needed help");
	Fallback.DistinguishingTrait = TraitStr;
	Fallback.FactionHistory = TEXT("Joined seeking protection after losing their previous group");
	Fallback.GenerationVersion = 0; // Mark as fallback

	// Add basic elements
	FRfsnBackstoryElement OriginElement;
	OriginElement.ElementType = TEXT("origin");
	OriginElement.Description = TEXT("Has been on the island since before the collapse");
	OriginElement.Importance = 0.8f;
	OriginElement.Tags.Add(TEXT("history"));
	OriginElement.bPublicKnowledge = true;
	Fallback.Elements.Add(OriginElement);

	FRfsnBackstoryElement TraitElement;
	TraitElement.ElementType = TEXT("personality");
	TraitElement.Description = TraitStr;
	TraitElement.Importance = 0.9f;
	TraitElement.Tags.Add(TEXT("personality"));
	TraitElement.bPublicKnowledge = true;
	Fallback.Elements.Add(TraitElement);

	return Fallback;
}

FString URfsnBackstoryGenerator::GetDialogueContext() const
{
	if (!HasBackstory())
	{
		return TEXT("");
	}

	// Build context string for LLM
	FString Context;
	Context += FString::Printf(TEXT("Background: %s\n"), *CachedBackstory.Summary);
	Context += FString::Printf(TEXT("Occupation: %s\n"), *CachedBackstory.Occupation);
	Context += FString::Printf(TEXT("Goal: %s\n"), *CachedBackstory.PersonalGoal);
	Context += FString::Printf(TEXT("Fear: %s\n"), *CachedBackstory.Fear);

	// Add public elements
	for (const FRfsnBackstoryElement& Element : CachedBackstory.Elements)
	{
		if (Element.bPublicKnowledge && Element.Importance > 0.5f)
		{
			Context += FString::Printf(TEXT("%s: %s\n"), *Element.ElementType, *Element.Description);
		}
	}

	return Context;
}

FString URfsnBackstoryGenerator::GetShortContext() const
{
	if (!HasBackstory())
	{
		return TEXT("");
	}

	// Just occupation and one distinguishing trait
	return FString::Printf(TEXT("A %s who is known for being %s."), *CachedBackstory.Occupation,
	                       *CachedBackstory.DistinguishingTrait);
}

FString URfsnBackstoryGenerator::GetElementByType(const FString& Type) const
{
	for (const FRfsnBackstoryElement& Element : CachedBackstory.Elements)
	{
		if (Element.ElementType.Equals(Type, ESearchCase::IgnoreCase))
		{
			return Element.Description;
		}
	}
	return TEXT("");
}

TArray<FRfsnBackstoryElement> URfsnBackstoryGenerator::GetElementsByTag(const FString& Tag) const
{
	TArray<FRfsnBackstoryElement> Result;

	for (const FRfsnBackstoryElement& Element : CachedBackstory.Elements)
	{
		if (Element.Tags.Contains(Tag))
		{
			Result.Add(Element);
		}
	}

	return Result;
}

void URfsnBackstoryGenerator::SaveBackstory()
{
	if (!HasBackstory())
	{
		return;
	}

	FString SlotName = GetSaveSlotName();

	// Save as JSON to a file
	TSharedRef<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("npc_id"), CachedBackstory.NpcId);
	JsonObj->SetStringField(TEXT("summary"), CachedBackstory.Summary);
	JsonObj->SetStringField(TEXT("occupation"), CachedBackstory.Occupation);
	JsonObj->SetStringField(TEXT("faction_history"), CachedBackstory.FactionHistory);
	JsonObj->SetStringField(TEXT("personal_goal"), CachedBackstory.PersonalGoal);
	JsonObj->SetStringField(TEXT("fear"), CachedBackstory.Fear);
	JsonObj->SetStringField(TEXT("secret"), CachedBackstory.SecretOrShame);
	JsonObj->SetStringField(TEXT("trait"), CachedBackstory.DistinguishingTrait);
	JsonObj->SetNumberField(TEXT("version"), CachedBackstory.GenerationVersion);

	// Elements array
	TArray<TSharedPtr<FJsonValue>> ElementsArray;
	for (const FRfsnBackstoryElement& Element : CachedBackstory.Elements)
	{
		TSharedRef<FJsonObject> ElementObj = MakeShared<FJsonObject>();
		ElementObj->SetStringField(TEXT("type"), Element.ElementType);
		ElementObj->SetStringField(TEXT("description"), Element.Description);
		ElementObj->SetNumberField(TEXT("importance"), Element.Importance);
		ElementObj->SetBoolField(TEXT("public"), Element.bPublicKnowledge);

		TArray<TSharedPtr<FJsonValue>> TagsArray;
		for (const FString& Tag : Element.Tags)
		{
			TagsArray.Add(MakeShared<FJsonValueString>(Tag));
		}
		ElementObj->SetArrayField(TEXT("tags"), TagsArray);

		ElementsArray.Add(MakeShared<FJsonValueObject>(ElementObj));
	}
	JsonObj->SetArrayField(TEXT("elements"), ElementsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObj, Writer);

	FString SavePath = FPaths::ProjectSavedDir() / TEXT("Backstories") / SlotName + TEXT(".json");
	FFileHelper::SaveStringToFile(OutputString, *SavePath);

	RFSN_LOG(TEXT("Saved backstory to %s"), *SavePath);
}

bool URfsnBackstoryGenerator::LoadBackstory()
{
	FString SlotName = GetSaveSlotName();
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("Backstories") / SlotName + TEXT(".json");

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *SavePath))
	{
		return false;
	}

	if (ParseBackstoryResponse(JsonString, CachedBackstory))
	{
		RFSN_LOG(TEXT("Loaded backstory from %s"), *SavePath);
		return true;
	}

	return false;
}

void URfsnBackstoryGenerator::ClearBackstory()
{
	CachedBackstory = FRfsnNpcBackstory();
	bHasInteracted = false;

	// Delete save file
	FString SlotName = GetSaveSlotName();
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("Backstories") / SlotName + TEXT(".json");
	IFileManager::Get().Delete(*SavePath);

	RFSN_LOG(TEXT("Cleared backstory for %s"), *GetOwner()->GetName());
}

bool URfsnBackstoryGenerator::DoesSaveExist() const
{
	FString SlotName = GetSaveSlotName();
	FString SavePath = FPaths::ProjectSavedDir() / TEXT("Backstories") / SlotName + TEXT(".json");
	return FPaths::FileExists(SavePath);
}

FString URfsnBackstoryGenerator::GetSaveSlotName() const
{
	FString NpcId = RfsnClient ? RfsnClient->NpcId : GetOwner()->GetName();
	return FString::Printf(TEXT("Backstory_%s"), *NpcId);
}

void URfsnBackstoryGenerator::SeedTemporalMemory(URfsnTemporalMemory* Memory)
{
	if (!Memory || !HasBackstory())
	{
		return;
	}

	// Create memory traces from backstory elements
	for (const FRfsnBackstoryElement& Element : CachedBackstory.Elements)
	{
		if (Element.Importance > 0.7f)
		{
			// High-importance elements become influential memories
			// This seeds the temporal memory with backstory context
			// The memory system will use these to influence future behavior

			// Note: This is a placeholder - actual implementation depends on
			// FRfsnMemoryTrace structure in TemporalMemory
			RFSN_LOG(TEXT("Seeding memory with %s element: %s"), *Element.ElementType, *Element.Description.Left(50));
		}
	}
}
