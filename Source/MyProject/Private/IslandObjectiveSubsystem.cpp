#include "IslandObjectiveSubsystem.h"

void UIslandObjectiveSubsystem::SetObjectiveActive(bool bActive, const FVector& Location)
{
	bObjectiveActive = bActive;
	ObjectiveLocation = bActive ? Location : FVector::ZeroVector;
}
