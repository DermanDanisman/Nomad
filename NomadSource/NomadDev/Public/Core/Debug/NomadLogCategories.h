// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// ========================================================================
// LOG CATEGORY DECLARATIONS
// ========================================================================

DECLARE_LOG_CATEGORY_EXTERN(LogNomadSurvival, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNomadSurvivalStats, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNomadSurvivalTemp, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNomadSurvivalEvents, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNomadAffliction, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNomadAfflictionConfig, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNomadAfflictionACF, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNomadPerformance, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNomadNetworking, Log, All);

// ========================================================================
// SIMPLE LOGGING MACROS
// ========================================================================

#define UE_LOG_SURVIVAL(Verbosity, Format, ...) \
    UE_LOG(LogNomadSurvival, Verbosity, TEXT("[%s] " Format), TEXT(__FUNCTION__), ##__VA_ARGS__)

#define UE_LOG_SURVIVAL_STATS(Verbosity, Format, ...) \
    UE_LOG(LogNomadSurvivalStats, Verbosity, TEXT("[%s] " Format), TEXT(__FUNCTION__), ##__VA_ARGS__)

#define UE_LOG_SURVIVAL_TEMP(Verbosity, Format, ...) \
    UE_LOG(LogNomadSurvivalTemp, Verbosity, TEXT("[%s] " Format), TEXT(__FUNCTION__), ##__VA_ARGS__)

#define UE_LOG_SURVIVAL_EVENTS(Verbosity, Format, ...) \
    UE_LOG(LogNomadSurvivalEvents, Verbosity, TEXT("[%s] " Format), TEXT(__FUNCTION__), ##__VA_ARGS__)

#define UE_LOG_AFFLICTION(Verbosity, Format, ...) \
    UE_LOG(LogNomadAffliction, Verbosity, TEXT("[%s] " Format), TEXT(__FUNCTION__), ##__VA_ARGS__)

#define UE_LOG_AFFLICTION_CONFIG(Verbosity, Format, ...) \
    UE_LOG(LogNomadAfflictionConfig, Verbosity, TEXT("[%s] " Format), TEXT(__FUNCTION__), ##__VA_ARGS__)

#define UE_LOG_AFFLICTION_ACF(Verbosity, Format, ...) \
    UE_LOG(LogNomadAfflictionACF, Verbosity, TEXT("[%s] " Format), TEXT(__FUNCTION__), ##__VA_ARGS__)

#define UE_LOG_NOMAD_PERF(Verbosity, Format, ...) \
    UE_LOG(LogNomadPerformance, Verbosity, TEXT("[%s] " Format), TEXT(__FUNCTION__), ##__VA_ARGS__)

#define UE_LOG_NOMAD_NET(Verbosity, Format, ...) \
    UE_LOG(LogNomadNetworking, Verbosity, TEXT("[%s] " Format), TEXT(__FUNCTION__), ##__VA_ARGS__)

#define UE_LOG_SURVIVAL_DEV(Verbosity, Format, ...) \
    UE_CLOG(!UE_BUILD_SHIPPING, LogNomadSurvival, Verbosity, TEXT("[DEV][%s] " Format), TEXT(__FUNCTION__), ##__VA_ARGS__)

// Simple function entry/exit logging
#define SURVIVAL_LOG_ENTER(FunctionName) \
    UE_LOG(LogNomadSurvival, Verbose, TEXT("ENTER: %s"), TEXT(FunctionName))

#define SURVIVAL_LOG_EXIT(FunctionName) \
    UE_LOG(LogNomadSurvival, Verbose, TEXT("EXIT: %s"), TEXT(FunctionName))