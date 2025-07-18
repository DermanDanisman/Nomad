// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "ARSTypes.h"
#include "NomadInstantEffectConfig.generated.h"

class UNomadBaseStatusEffect;

/**
 * UNomadInstantEffectConfig
 * -------------------------
 * Configuration for instant (one-shot) status effects.
 *
 * Key Features:
 * - Apply immediately and end
 * - No duration or persistence
 * - Supports chain effects
 * - Full hybrid system integration
 * - Optimized for quick application
 *
 * Use Cases:
 * - Healing potions
 * - Damage spells
 * - Instant buffs/debuffs
 * - Trigger effects
 * - Resource modifications
 */
UCLASS(BlueprintType, meta=(DisplayName="Instant Effect Config"))
class NOMADDEV_API UNomadInstantEffectConfig : public UNomadStatusEffectConfigBase
{
    GENERATED_BODY()

public:
    UNomadInstantEffectConfig();

    // =====================================================
    //         INSTANT EFFECT SETTINGS
    // =====================================================

    /** Should trigger screen effects for extra feedback? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Instant Effect", meta=(
        ToolTip="Enable screen flash, shake, or other dramatic feedback"))
    bool bTriggerScreenEffects = false;

    /** Should this effect bypass standard cooldowns? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Instant Effect", meta=(
        ToolTip="Allow rapid successive applications"))
    bool bBypassCooldowns = false;

    // =====================================================
    //         STAT MODIFICATIONS (HYBRID SYSTEM)
    // =====================================================

    /** Main stat modifications to apply instantly */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent",
        EditConditionHides, ToolTip="Immediate stat changes when effect is applied"))
    TArray<FStatisticValue> OnApplyStatModifications;

    /** Temporary attribute modifier (applied and immediately removed) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent",
        EditConditionHides, ToolTip="Brief attribute changes for calculation purposes"))
    FAttributesSetModifier TemporaryAttributeModifier;

    // =====================================================
    //         CHAIN EFFECTS
    // =====================================================

    /** Should this effect trigger additional effects? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(
        ToolTip="Trigger other status effects after this one completes"))
    bool bTriggerChainEffects = false;

    /** Effects to trigger after completion */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(
        EditCondition="bTriggerChainEffects",
        ToolTip="Additional effects applied after this instant effect"))
    TArray<TSoftClassPtr<UNomadBaseStatusEffect>> ChainEffects;

    /** Delay before triggering chain effects (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(
        EditCondition="bTriggerChainEffects", ClampMin="0.0",
        ToolTip="Time to wait before applying chain effects"))
    float ChainEffectDelay = 0.0f;

    // =====================================================
    //         FEEDBACK SETTINGS
    // =====================================================

    /** Should show floating combat text? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback", meta=(
        ToolTip="Display damage/healing numbers"))
    bool bShowFloatingText = true;

    /** Should interrupt other effects when applied? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback", meta=(
        ToolTip="Cancel certain ongoing effects when this is applied"))
    bool bInterruptsOtherEffects = false;

    /** Tags of effects to interrupt */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback", meta=(
        EditCondition="bInterruptsOtherEffects",
        ToolTip="Effects with these tags will be interrupted"))
    FGameplayTagContainer InterruptTags;

public:
    // =====================================================
    //         VALIDATION & INFO
    // =====================================================

    /** Validate configuration */
    virtual bool IsConfigValid() const override;

    /** Get validation errors */
    virtual TArray<FString> GetValidationErrors() const override;

    /** Returns effect type description */
    virtual FText GetEffectTypeDescription() const override
    {
        return FText::FromString(TEXT("Instant Effect"));
    }

    /** Get effect magnitude for UI display */
    UFUNCTION(BlueprintPure, Category="Configuration")
    float GetEffectMagnitude() const;

protected:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};