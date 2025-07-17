// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "ARSTypes.h"
#include "NomadInstantEffectConfig.generated.h"

/**
 * UNomadInstantEffectConfig
 * -------------------------
 * Configuration asset for instant (one-shot) status effects.
 * 
 * Key Features:
 * - Data-driven: All instant effect logic (stat changes, chain effects, UI, etc.) is configured here.
 * - Rapid designer iteration: No code changes needed to author new instant effects.
 * - Fully supports Blueprint and C++ workflow.
 * - HYBRID SYSTEM: Supports stat modification, damage event, or both (set in ApplicationMode).
 * 
 * Typical use: Healing bursts, direct damage, instant buffs/debuffs, triggers for chain reactions, and any non-persistent effect.
 */
UCLASS(BlueprintType, meta=(DisplayName="Instant Effect Config"))
class NOMADDEV_API UNomadInstantEffectConfig : public UNomadStatusEffectConfigBase
{
    GENERATED_BODY()

public:
    UNomadInstantEffectConfig();

    // ======== Instant Effect Settings ========

    /** Should trigger visual/screen effects (flash, shake, etc.) for extra feedback. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Instant Effect")
    bool bTriggerScreenEffects = false;

    // ======== Stat Modifications & Damage Hybrid ========

    /**
     * Stat modifications or damage to apply instantly.
     * - If ApplicationMode is StatModification, applies as stat mods.
     * - If ApplicationMode is DamageEvent, applies as damage (using DamageTypeClass from base).
     * - If ApplicationMode is Both, applies both.
     * 
     * NOTE: OnApplyStatModifications is the canonical hybrid field for instant effects.
     *       InstantStatModifications is kept for backward compatibility and is always used as OnApplyStatModifications.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent", EditConditionHides))
    TArray<FStatisticValue> OnApplyStatModifications;

    // Backward compatibility: Synonym for OnApplyStatModifications.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        DeprecatedProperty, DisplayName="Instant Stat Modifications", EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent", EditConditionHides))
    TArray<FStatisticValue> InstantStatModifications;

    /**
     * Persistent attribute/primary/stat modifier to apply instantly.
     * Useful for temporary stat boosts (removed after effect ends).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent", EditConditionHides))
    FAttributesSetModifier AttributeModifier;

    // ======== Chain Effects ========

    /** Should this effect trigger additional status effects when executed? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects")
    bool bTriggerChainEffects = false;

    /** Classes of status effects to trigger after this one completes (if enabled). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(EditCondition="bTriggerChainEffects"))
    TArray<TSoftClassPtr<UNomadBaseStatusEffect>> ChainEffects;

    // ======== Overrides ========

    /** Returns a type description for asset browsers, tooltips, etc. */
    virtual FText GetEffectTypeDescription() const override 
    { 
        return FText::FromString(TEXT("Instant Effect")); 
    }
    
#if WITH_EDITOR
    /** Editor-side validation of config properties. */
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};