// MyProject Shared PCH Header
// Include commonly used headers here for faster compilation

#pragma once

// ─────────────────────────────────────────────────────────────
// Core UE Headers
// ─────────────────────────────────────────────────────────────
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

// ─────────────────────────────────────────────────────────────
// Common Gameplay Headers
// ─────────────────────────────────────────────────────────────
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"

// ─────────────────────────────────────────────────────────────
// Input
// ─────────────────────────────────────────────────────────────
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

// ─────────────────────────────────────────────────────────────
// Subsystems
// ─────────────────────────────────────────────────────────────
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/WorldSubsystem.h"

// ─────────────────────────────────────────────────────────────
// Common Utilities
// ─────────────────────────────────────────────────────────────
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// ─────────────────────────────────────────────────────────────
// RFSN Common (used across many RFSN components)
// ─────────────────────────────────────────────────────────────
// Forward declarations for RFSN types
class URfsnNpcClientComponent;
class URfsnDialogueManager;
struct FRfsnSentence;
