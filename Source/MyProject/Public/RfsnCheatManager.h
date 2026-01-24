// RFSN Console Commands
// Debug commands for RFSN system testing

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "RfsnCheatManager.generated.h"

/**
 * Cheat manager extension for RFSN debug commands.
 * Add to your GameMode or PlayerController.
 */
UCLASS()
class MYPROJECT_API URfsnCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────
	// Debug Commands
	// ─────────────────────────────────────────────────────────────

	/** Toggle RFSN debug HUD overlay */
	UFUNCTION(Exec)
	virtual void RfsnDebug();

	/** Test dialogue with nearest NPC */
	UFUNCTION(Exec)
	virtual void RfsnTalk();

	/** Send custom message to active NPC */
	UFUNCTION(Exec)
	virtual void RfsnSay(const FString& Message);

	/** End current dialogue */
	UFUNCTION(Exec)
	virtual void RfsnEndDialogue();

	/** Check RFSN server connection */
	UFUNCTION(Exec)
	virtual void RfsnPingServer();

	/** List all RFSN NPCs in level */
	UFUNCTION(Exec)
	virtual void RfsnListNpcs();

	/** Set NPC mood by name */
	UFUNCTION(Exec)
	virtual void RfsnSetMood(const FString& NpcName, const FString& Mood);

	/** Spawn test NPC at player location */
	UFUNCTION(Exec)
	virtual void RfsnSpawnNpc(const FString& NpcType);

	/** Toggle mock server mode (offline testing) */
	UFUNCTION(Exec)
	virtual void RfsnMockMode();

	/** Dump current dialogue log */
	UFUNCTION(Exec)
	virtual void RfsnDumpLog();

private:
	bool bMockModeEnabled = false;
};
