// MyProject Shared PCH Header
// Include commonly used headers here for faster compilation

#pragma once

// ─────────────────────────────────────────────────────────────
// Core UE Headers
// ─────────────────────────────────────────────────────────────
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"

// ─────────────────────────────────────────────────────────────
// Common Gameplay Headers
// ─────────────────────────────────────────────────────────────
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

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
// Networking (for replicated components)
// ─────────────────────────────────────────────────────────────
#include "Net/UnrealNetwork.h"

// ─────────────────────────────────────────────────────────────
// HTTP/JSON (for RFSN communication)
// ─────────────────────────────────────────────────────────────
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// ─────────────────────────────────────────────────────────────
// Common Utilities
// ─────────────────────────────────────────────────────────────
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"
#include "EngineUtils.h"

// ─────────────────────────────────────────────────────────────
// RFSN Forward Declarations
// ─────────────────────────────────────────────────────────────
#include "RfsnForwardDeclarations.h"
#include "RfsnLogging.h"
