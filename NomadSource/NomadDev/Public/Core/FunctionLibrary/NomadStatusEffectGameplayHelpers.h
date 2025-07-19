// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NomadStatusEffectGameplayHelpers.generated.h"

// Forward declarations to reduce coupling
enum class ESurvivalSeverity : uint8;

/**
 * Utility library for syncing ARS/ACF MovementSpeed attribute with Character movement.
 * - Call after any stat/attribute mod (buff, debuff, slow, haste, etc.).
 * - Works with both percentage-based and additive mods: make sure your MovementSpeed attribute is updated properly!
 * - Call from C++ or Blueprint (e.g., on stat change, on status effect applied/removed, on item equip/unequip).
 */
UCLASS(Blueprintable)
class NOMADDEV_API UNomadStatusEffectGameplayHelpers : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    
};