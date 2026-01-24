// RFSN Logging Categories
// Declare logging categories for RFSN subsystem

#pragma once

#include "CoreMinimal.h"

// ─────────────────────────────────────────────────────────────
// RFSN Logging Categories
// ─────────────────────────────────────────────────────────────

// Main RFSN logging category
DECLARE_LOG_CATEGORY_EXTERN(LogRfsn, Log, All);

// Specific subsystem categories
DECLARE_LOG_CATEGORY_EXTERN(LogRfsnDialogue, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogRfsnDirector, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogRfsnHttp, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogRfsnAudio, Log, All);

// ─────────────────────────────────────────────────────────────
// Logging Macros for cleaner code
// ─────────────────────────────────────────────────────────────

#define RFSN_LOG(Format, ...) UE_LOG(LogRfsn, Log, TEXT(Format), ##__VA_ARGS__)

#define RFSN_WARNING(Format, ...)                                              \
  UE_LOG(LogRfsn, Warning, TEXT(Format), ##__VA_ARGS__)

#define RFSN_ERROR(Format, ...)                                                \
  UE_LOG(LogRfsn, Error, TEXT(Format), ##__VA_ARGS__)

#define RFSN_VERBOSE(Format, ...)                                              \
  UE_LOG(LogRfsn, Verbose, TEXT(Format), ##__VA_ARGS__)

// Dialogue-specific
#define RFSN_DIALOGUE_LOG(Format, ...)                                         \
  UE_LOG(LogRfsnDialogue, Log, TEXT(Format), ##__VA_ARGS__)

// Director-specific
#define RFSN_DIRECTOR_LOG(Format, ...)                                         \
  UE_LOG(LogRfsnDirector, Log, TEXT(Format), ##__VA_ARGS__)

// HTTP-specific
#define RFSN_HTTP_LOG(Format, ...)                                             \
  UE_LOG(LogRfsnHttp, Log, TEXT(Format), ##__VA_ARGS__)

// Audio-specific
#define RFSN_AUDIO_LOG(Format, ...)                                            \
  UE_LOG(LogRfsnAudio, Log, TEXT(Format), ##__VA_ARGS__)
