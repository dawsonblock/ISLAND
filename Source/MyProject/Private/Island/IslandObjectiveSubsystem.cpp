#include "Island/IslandObjectiveSubsystem.h"

void UIslandObjectiveSubsystem::SetObjectiveActive(bool bActive, const FVector& Location)
{
	bObjectiveActive = bActive;
	ObjectiveLocation = bActive ? Location : FVector::ZeroVector;
}

void UIslandObjectiveSubsystem::SetObjectiveText(const FText& NewText)
{
	CurrentObjectiveText = NewText;
}
