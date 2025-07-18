// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Data/StatusEffect/NomadInstantEffectConfig.h"
#include "NomadInstantStatusEffect.generated.h"

// Forward declarations
class UNomadBaseStatusEffect;

// =====================================================
//         CLASS DECLARATION: INSTANT STATUS EFFECT
// =====================================================

/**
 * UNomadInstantStatusEffect
 * -------------------------
 * Data-driven, instant (one-shot) status effect for immediate gameplay application.
 *
 * Key Features:
 * - Applies its effect immediately on creation
 * - Does NOT persist, does not tick, not tracked by manager for stacking
 * - Supports chain effects with optional delays
 * - Can interrupt other effects
 * - Full hybrid system integration
 * - Temporary attribute modifiers for brief calculations
 * - Screen effects and floating text support
 * - Smart removal system compatible (though rarely needed)
 *
 * Perfect for:
 * - Healing potions and damage spells
 * - Instant stat modifications
 * - Trigger effects and gameplay events
 * - Resource modifications (mana, stamina)
 * - Interrupt abilities
 *
 * Design Philosophy:
 * - Apply immediately and end (no persistence)
 * - Chain effects for complex interactions
 * - Interrupt system for tactical gameplay
 * - Rich feedback for immediate impact
 * - Manager integration for analytics only
 *
 * Use Cases:
 * - Instant healing: +100 Health
 * - Damage spells: -50 Health via damage event
 * - Resource restore: +50 Mana
 * - Interrupt: Cancel enemy casting
 * - Trigger: Activate other effects after delay
 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="Instant Status Effect"))
class NOMADDEV_API UNomadInstantStatusEffect : public UNomadBaseStatusEffect
{
    GENERATED_BODY()

public:
    // =====================================================
    //         CONSTRUCTOR & INITIALIZATION
    // =====================================================
    UNomadInstantStatusEffect();

    // =====================================================
    //         CONFIGURATION ACCESS
    // =====================================================

    /** Loads and returns the config asset, or nullptr if not set/invalid */
    virtual UNomadInstantEffectConfig* GetEffectConfig() const override;

    /** Returns true if configuration is loaded and valid */
    UFUNCTION(BlueprintPure, Category="Nomad Instant Effect | Configuration")
    bool HasValidConfiguration() const;

    // =====================================================
    //         MANAGER ACTIVATION ENTRYPOINT
    // =====================================================

    /** Called by the effect manager to trigger instant effect logic polymorphically */
    virtual void Nomad_OnStatusEffectStarts(ACharacter* Character) override;

    // =====================================================
    //         RUNTIME STATE & ANALYTICS
    // =====================================================

    /** Last amount of damage (or healing) applied - for analytics/UI */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    float LastAppliedValue = 0.0f;

    /** Gets the last applied value for UI/analytics */
    UFUNCTION(BlueprintPure, Category="Nomad Instant Effect | Query")
    float GetLastAppliedValue() const { return LastAppliedValue; }

    /** Gets the magnitude of the effect for UI display */
    UFUNCTION(BlueprintPure, Category="Nomad Instant Effect | Query")
    float GetEffectMagnitude() const;

protected:
    // =====================================================
    //         CORE LIFECYCLE OVERRIDES
    // =====================================================

    /** Called when the effect is created/applied. Handles all logic and ends itself */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /** Called when the effect ends (after instant application) */
    virtual void OnStatusEffectEnds_Implementation() override;

    // =====================================================
    //         HYBRID SYSTEM: STAT/DAMAGE/BOTH APPLICATION
    // =====================================================

    /** Applies stat mods and/or damage according to the config's ApplicationMode */
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfig) override;

    // =====================================================
    //         INSTANT EFFECT EVENTS (BLUEPRINT HOOKS)
    // =====================================================

    /** Called after instant effect is applied, for VFX/SFX/UI */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Instant Effect | Events")
    void OnInstantEffectApplied(const TArray<FStatisticValue>& Modifications);
    virtual void OnInstantEffectApplied_Implementation(const TArray<FStatisticValue>& Modifications);

    /** Called when the effect finishes (for custom cleanup/SFX) */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Instant Effect | Events")
    void OnInstantEffectEnded();
    virtual void OnInstantEffectEnded_Implementation();

    /** Called when chain effects are about to be triggered */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Instant Effect | Events")
    void OnChainEffectsTriggered(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects);

    /** Called when other effects are interrupted by this effect */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Instant Effect | Events")
    void OnEffectsInterrupted(const FGameplayTagContainer& InterruptedTags);

private:
    // =====================================================
    //         INTERNAL HELPERS
    // =====================================================

    /** Applies chain effects through the status effect manager */
    void ApplyChainEffects(ACharacter* Character, UNomadInstantEffectConfig* Config);

    /** Calculates and returns the total magnitude of this effect */
    float CalculateEffectMagnitude() const;
};