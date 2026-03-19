#include "Island/IslandHUDWidget.h"

void UIslandHUDWidget::ApplyHudState(const FIslandHUDState& NewState)
{
	CurrentState = NewState;
	BP_HudStateUpdated();
}
