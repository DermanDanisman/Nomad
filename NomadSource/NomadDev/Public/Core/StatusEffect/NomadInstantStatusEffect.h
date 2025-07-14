// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Data/StatusEffect/NomadInstantEffectConfig.h"
#include "NomadInstantStatusEffect.generated.h"

// =====================================================
//         CLASS DECLARATION: INSTANT STATUS EFFECT
// =====================================================

/**
 * UNomadInstantStatusEffect
 * ----------------------------------------------------------
 * Data-driven, instant (one-shot) status effect for immediate gameplay/application.
 * - Applies its effect immediately on creation (e.g., burst heal, hit, instant buff/debuff, instant stat mod).
 * - Does NOT persist, does not tick, and is not tracked by manager for stacking.
 * - Used for instant healing, damage, instant stat or attribute changes, triggers, or gameplay events.
 * - All logic and configuration comes from a UNomadInstantEffectConfig asset.
 * - Once application is complete, the effect destroys itself (or is GC'd).
 *
 * Design Notes:
 *  - No stacking/tick/duration logic. Effect is applied and then ends itself.
 *  - Integrates with the Nomad hybrid stat/damage system for analytics/UI.
 *  - Notifies manager only for analytics (if needed).
 *  - Blueprint events can be used for VFX/SFX/UI triggers.
 *  - Use for any event that should not persist or be tracked over time.
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
    
    /** Loads and returns the config asset, or nullptr if not set/invalid. */
    virtual UNomadInstantEffectConfig* GetEffectConfig() const override;

    /** Returns true if configuration is loaded and valid. */
    UFUNCTION(BlueprintPure, Category="Configuration")
    bool HasValidConfiguration() const;

    // =====================================================
    //         MANAGER ACTIVATION ENTRYPOINT
    // =====================================================
    /**
     * Called by the effect manager (or scripts) to trigger instant effect logic in a
     * consistent and extensible way. This enables manager code to activate any subclass
     * of NomadBaseStatusEffect (including instant, timed, infinite) without needing to know
     * the specific subclass at compile time.
     */
    virtual void Nomad_OnStatusEffectStarts(ACharacter* Character) override;

    // =====================================================
    //         CONFIGURATION (ASSET-DRIVEN)
    // =====================================================;

    // =====================================================
    //         RUNTIME STATE
    // =====================================================

    /** Last amount of damage (or healing) applied - for analytics/UI. */
    UPROPERTY()
    float LastAppliedValue = 0.0f;
    
protected:

    // =====================================================
    //         CORE LIFECYCLE OVERRIDES
    // =====================================================
    
    /** Called when the effect is created/applied. Handles all logic and ends itself. */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /** Called when the effect ends (after instant application). */
    virtual void OnStatusEffectEnds_Implementation() override;
    
    // =====================================================
    //         HYBRID SYSTEM: STAT/DAMAGE/BOTH APPLICATION
    // =====================================================
    
    /** Applies stat mods and/or damage according to the config's ApplicationMode. */
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfig) override;

    // =====================================================
    //         INSTANT EFFECT EVENTS (BLUEPRINT HOOKS)
    // =====================================================

    /** Called after instant effect is applied, for VFX/SFX/UI. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Instant Effect")
    void OnInstantEffectApplied(const TArray<FStatisticValue>& Modifications);
    virtual void OnInstantEffectApplied_Implementation(const TArray<FStatisticValue>& Modifications);

    /** Called when the effect finishes (for custom cleanup/SFX). */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Instant Effect")
    void OnInstantEffectEnded();
    virtual void OnInstantEffectEnded_Implementation();
};