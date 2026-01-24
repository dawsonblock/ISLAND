// RFSN Sample NPC - Guard with hostile/neutral dialogue
// Demonstrates RFSN client for checkpoint/patrol guard

#pragma once

#include "CoreMinimal.h"
#include "Variant_Shooter/AI/ShooterNPC.h"
#include "RfsnSampleGuard.generated.h"

/**
 * Sample guard NPC with RFSN dialogue.
 * Can be hostile or neutral based on player actions.
 * Inherits from ShooterNPC for combat capability.
 */
UCLASS(Blueprintable)
class MYPROJECT_API ARfsnSampleGuard : public AShooterNPC
{
	GENERATED_BODY()

public:
	ARfsnSampleGuard();

	/** Guard patrol mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	bool bOnPatrol = true;

	/** Alert radius - larger than dialogue radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	float AlertRadius = 500.0f;

	/** Has the guard been warned? */
	UPROPERTY(BlueprintReadOnly, Category = "Guard")
	bool bPlayerWarned = false;

protected:
	virtual void BeginPlay() override;

	/** Handle RFSN action - override from parent */
	virtual void OnRfsnNpcAction(ERfsnNpcAction Action) override;

private:
	void ConfigureAsHostileGuard();
	void ConfigureAsNeutralGuard();
};
