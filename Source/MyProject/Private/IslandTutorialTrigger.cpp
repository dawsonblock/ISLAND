#include "IslandTutorialTrigger.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/HUD.h"
#include "IslandHUD.h"

AIslandTutorialTrigger::AIslandTutorialTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void AIslandTutorialTrigger::BeginPlay()
{
	Super::BeginPlay();
	
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AIslandTutorialTrigger::OnOverlapBegin);
}

void AIslandTutorialTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bOneShot && bHasTriggered)
	{
		return;
	}

	if (OtherActor && OtherActor->IsA(ACharacter::StaticClass()))
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC && PC->GetHUD())
		{
			if (AIslandHUD* IslandHUD = Cast<AIslandHUD>(PC->GetHUD()))
			{
				IslandHUD->ShowTutorialMessage(Message, Duration);
				bHasTriggered = true;

				if (bOneShot)
				{
					// Optional: Destroy self or disable collision to save performance
					// Destroy(); 
				}
			}
		}
	}
}
