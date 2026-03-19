// RFSN NPC Config Implementation

#include "Core/RfsnNpcConfig.h"
#include "Dialogue/RfsnNpcClientComponent.h"
#include "Dialogue/RfsnNpcDialogueTrigger.h"
#include "Social/RfsnNpcLookAt.h"
#include "Dialogue/RfsnDialogueCamera.h"
#include "Social/RfsnAmbientChatter.h"
#include "Core/RfsnAudioSettings.h"
#include "Core/RfsnLogging.h"

void URfsnNpcConfig::ApplyToNpc(URfsnNpcClientComponent* Client)
{
	if (!Client)
	{
		RFSN_ERROR(TEXT("Cannot apply config - no client component"));
		return;
	}

	AActor* Owner = Client->GetOwner();
	if (!Owner)
	{
		RFSN_ERROR(TEXT("Cannot apply config - no owner actor"));
		return;
	}

	// Apply identity
	Client->NpcId = NpcId;
	Client->NpcName = DisplayName;
	Client->TtsEngine = TtsEngine;

	// Apply personality
	Client->Mood = DefaultMood;
	Client->Relationship = InitialRelationship;
	Client->Affinity = InitialAffinity;

	// Configure dialogue trigger if present
	if (URfsnNpcDialogueTrigger* Trigger = Owner->FindComponentByClass<URfsnNpcDialogueTrigger>())
	{
		if (bProximityDialogue)
		{
			Trigger->TriggerMode = ERfsnDialogueTriggerMode::Proximity;
			Trigger->ProximityRadius = DialogueRadius;
			Trigger->TriggerCooldown = DialogueCooldown;
			Trigger->DefaultPrompt = GreetingLine;
		}
		else
		{
			Trigger->TriggerMode = ERfsnDialogueTriggerMode::Interact;
		}
	}

	// Configure look-at if present
	if (URfsnNpcLookAt* LookAt = Owner->FindComponentByClass<URfsnNpcLookAt>())
	{
		LookAt->bEnabled = bLookAtPlayer;
	}

	// Configure dialogue camera if present
	if (URfsnDialogueCamera* Camera = Owner->FindComponentByClass<URfsnDialogueCamera>())
	{
		Camera->bEnabled = bDialogueCamera;
	}

	// Configure ambient chatter if present
	if (URfsnAmbientChatter* Chatter = Owner->FindComponentByClass<URfsnAmbientChatter>())
	{
		Chatter->bEnabled = bEnableChatter;
		Chatter->ChatterLines = ChatterLines;
	}

	RFSN_LOG(TEXT("Applied config to NPC: %s (Faction: %s)"), *DisplayName, *FactionId);
}
