// RFSN Forward Declarations
// Use this header instead of including full headers when possible
// Reduces compile times by avoiding unnecessary header parsing

#pragma once

// ─────────────────────────────────────────────────────────────
// Core RFSN Types (forward declarations)
// ─────────────────────────────────────────────────────────────

// Core Components
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
class URfsnRelationshipManager;
class URfsnFactionSystem;
class URfsnNpcConversation;
class URfsnHttpPool;
class URfsnMetrics;

// Save Data
class URfsnRelationshipSaveData;

// Enhancement Components
class URfsnNpcLookAt;
class URfsnDialogueCamera;
class URfsnAmbientChatter;
class URfsnAudioSettings;
class URfsnReplicatedDialogue;

// Advanced AI Components
class URfsnEmotionBlend;
class URfsnBackstoryGenerator;
class URfsnTemporalMemory;
class URfsnActionLattice;

// UI Components
class URfsnNpcPortrait;
class URfsnReputationHud;

// Advanced Systems
class URfsnWitnessSystem;
class URfsnLipSync;
class URfsnNpcSchedule;

// Configuration
class URfsnNpcConfig;

// Blueprint Library
class URfsnBlueprintLibrary;
class URfsnCheatManager;

// Sample NPCs
class ARfsnSampleMerchant;
class ARfsnSampleGuard;

// ─────────────────────────────────────────────────────────────
// Structs - forward declare where possible
// ─────────────────────────────────────────────────────────────

struct FRfsnSentence;
struct FRfsnDialogueMeta;
struct FRfsnConversationEntry;
struct FRfsnNpcRelationship;
struct FRfsnFaction;
struct FRfsnChatterLine;
struct FRfsnNpcConversationSession;
struct FRfsnNpcConversationParticipant;
struct FRfsnEmotionAxis;
struct FRfsnFacialExpression;
struct FRfsnNpcBackstory;
struct FRfsnBackstoryElement;
struct FRfsnExpandedAction;
struct FRfsnMemoryTrace;
struct FRfsnActionBias;

// ─────────────────────────────────────────────────────────────
// Enums - typically need full header for usage
// ─────────────────────────────────────────────────────────────
// ERfsnNpcAction - see RfsnNpcClientComponent.h
// ERfsnDialogueTriggerMode - see RfsnNpcDialogueTrigger.h
// ERfsnCameraMode - see RfsnDialogueCamera.h
// ERfsnChatterTrigger - see RfsnAmbientChatter.h
// ERfsnConversationType - see RfsnNpcConversation.h
// ERfsnCoreEmotion - see RfsnEmotionBlend.h
// ERfsnActionIntensity - see RfsnActionLattice.h
// ERfsnActionCompliance - see RfsnActionLattice.h
// ERfsnActionMotive - see RfsnActionLattice.h
