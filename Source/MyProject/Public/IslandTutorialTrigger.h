#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandTutorialTrigger.generated.h"

class UBoxComponent;

UCLASS()
class MYPROJECT_API AIslandTutorialTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	AIslandTutorialTrigger();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UBoxComponent* TriggerBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tutorial", meta=(MultiLine="true"))
	FString Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tutorial")
	float Duration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tutorial")
	bool bOneShot = true;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bHasTriggered = false;
};
