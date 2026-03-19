// RFSN Sample NPC - Guard with hostile/neutral dialogue
// Demonstrates RFSN client for checkpoint/patrol guard

#pragma once

#include "CoreMinimal.h"
#include "Core/MyProjectCharacter.h"
#include "Dialogue/RfsnNpcClientComponent.h"
#include "RfsnSampleGuard.generated.h"

/**
 * Sample guard NPC with RFSN dialogue.
 * Can be hostile or neutral based on player actions.
 */
UCLASS(Blueprintable)
class MYPROJECT_API ARfsnSampleGuard : public AMyProjectCharacter
{
	GENERATED_BODY()

public:
	ARfsnSampleGuard();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RFSN")
	TObjectPtr<URfsnNpcClientComponent> RfsnClient;

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

	UFUNCTION()
	void OnRfsnNpcAction(ERfsnNpcAction Action);

private:
	void ConfigureAsHostileGuard();
	void ConfigureAsNeutralGuard();
};
