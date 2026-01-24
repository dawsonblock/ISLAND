// RFSN Forward Declarations
// Use this header instead of including full headers when possible
// Reduces compile times by avoiding unnecessary header parsing

#pragma once

// ─────────────────────────────────────────────────────────────
// Core RFSN Types (forward declarations)
// ─────────────────────────────────────────────────────────────

// Components
class URfsnNpcClientComponent;
class URfsnDirectorBridge;
class URfsnDialogueWidget;
class URfsnNpcDialogueTrigger;
class URfsnTtsAudioComponent;
class URfsnConversationLog;
class URfsnDebugHud;
class URfsnQuestBridge;

// Widgets
class URfsnPlayerInputWidget;
class URfsnChoiceWidget;

// Subsystems
class URfsnDialogueManager;

// Save Data
class URfsnRelationshipSaveData;

// Structs - these may need full header for usage
struct FRfsnSentence;
struct FRfsnMeta;
struct FRfsnConversationEntry;
struct FRfsnNpcRelationship;

// ─────────────────────────────────────────────────────────────
// Enums - forward declare if possible
// Note: Enums typically need full header for usage
// ─────────────────────────────────────────────────────────────
// ERfsnNpcAction - see RfsnNpcClientComponent.h
// ERfsnDialogueTriggerMode - see RfsnNpcDialogueTrigger.h
