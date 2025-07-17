// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StatusEffects/ACFBaseStatusEffect.h"
#include "Core/StatusEffect/NomadStatusTypes.h"
#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundBase.h"
#include "NomadBaseStatusEffect.generated.h"

class UNomadStatusEffectManagerComponent;
struct FStatisticValue;
class ACharacter;

// Canonical tag for health stat modifications
static const FGameplayTag Health = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.Health"));

UENUM(BlueprintType)
enum class EEffectLifecycleState : uint8
{
    Active,     // Effect is running
    Ending,     // Effect is being cleaned up
    Removed     // Effect is fully finished/cleaned
};

// =====================================================
//        CLASS DECLARATION: BASE STATUS EFFECT
// =====================================================

/**
 * UNomadBaseStatusEffect
 * -----------------------------------------------------
 * Abstract base class for all Nomad status effects.
 * 
 * Key features and responsibilities:
 *  - Data-driven: All gameplay and UI configuration comes from a config asset (BaseConfig).
 *  - Integration: Extends the ACF status effect system with Nomad-specific logic (tag, icons, category, sounds).
 *  - Audio/Visual: Handles start/end sound playback and exposes hooks for visual/audio cues.
 *  - Categorization: Every status effect can be sorted/categorized for UI, logic, or filtering.
 *  - Initialization: Prevents double-init and makes sure config is loaded/applied before effect logic runs.
 *  - **Hybrid system ready:** Each effect can apply via stat modification, damage event, or both, as set in config.
 * 
 * Design notes:
 *  - The effect itself is **not** responsible for UI notification/affliction popups. Only the manager triggers UI.
 *  - All config-driven properties are **read at runtime** from the config asset, supporting easy tuning.
 *  - Blueprint designers should always subclass this, not ACFBaseStatusEffect directly.
 *  
 * ===============================================
 *     NomadBaseStatusEffect (Hybrid System Ready)
 * ===============================================
 *
 * This is the abstract base class for all Nomad status effects—timed, instant, infinite.
 * It supports the hybrid system for stat mods and damage events, as defined in the config asset.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta=(DisplayName="Nomad Base Status Effect"))
class NOMADDEV_API UNomadBaseStatusEffect : public UACFBaseStatusEffect
{
    GENERATED_BODY()

public:
    // =====================================================
    //         CONSTRUCTOR & INITIALIZATION
    // =====================================================

    /** Default constructor: initializes runtime state. */
    UNomadBaseStatusEffect();

    UFUNCTION(BlueprintCallable, Category = "Nomad Status Effect | Getters")
    EEffectLifecycleState GetEffectLifecycleState() const
    { return EffectState; }

    UFUNCTION(BlueprintCallable, Category = "Nomad Status Effect | Setters")
    void SetEffectLifecycleState(const EEffectLifecycleState NewState) { EffectState = NewState; }

    UFUNCTION(BlueprintCallable, Category = "Nomad Status Effect | Setters")
    void SetDamageCauser(AActor* Causer)
    {
        DamageCauser = Causer;
    }

    // =====================================================
    //         DATA-DRIVEN CONFIGURATION
    // =====================================================

    /**
     * The configuration asset containing all gameplay/UI parameters for this effect.
     * - Should point to a UNomadStatusEffectConfigBase (BP or C++).
     * - If set, overrides any hardcoded properties.
     * - Loaded synchronously at runtime if not already loaded.
     * - Allows multiple effect instances to share config.
     * - **Hybrid:** Determines application mode for stat/damage/both.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Nomad Status Effect | Configuration")
    TSoftObjectPtr<UNomadStatusEffectConfigBase> EffectConfig;

protected:

    // =====================================================
    //         RUNTIME STATE
    // =====================================================

    /** True if this effect has already been initialized. Prevents accidental double-initialization. */
    UPROPERTY(BlueprintReadOnly, Category="Nomad Status Effect | Runtime")
    bool bIsInitialized = false;

    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Runtime")
    void ApplySprintBlockTag(ACharacter* Character);

    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Runtime")
    void RemoveSprintBlockTag(ACharacter* Character);

public:
    // =====================================================
    //         CONFIGURATION ACCESS & APPLICATION
    // =====================================================

    /** Loads and returns the config asset for this effect, or nullptr if not set. */
    UFUNCTION(BlueprintPure, Category="Nomad Status Effect | Configuration")
    virtual UNomadStatusEffectConfigBase* GetEffectConfig() const;

    /** Applies all configuration values from the config asset to this effect instance. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Configuration")
    virtual void ApplyBaseConfiguration();

    /** Returns true if the base configuration is set and valid. */
    UFUNCTION(BlueprintPure, Category="Nomad Status Effect | Configuration")
    bool HasValidBaseConfiguration() const;

    // =====================================================
    //         STATUS EFFECT PROPERTIES (Category, Tag, Icon)
    // =====================================================

    /** Returns the effect's gameplay category (debuff, buff, neutral, etc.). */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Nomad Status Effect | Status Effect")
    ENomadStatusCategory GetStatusCategory() const;
    virtual ENomadStatusCategory GetStatusCategory_Implementation() const;

    /** Applies the gameplay tag from the config asset to this effect instance. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Configuration")
    void ApplyTagFromConfig();

    /** Applies the icon from the config asset to this effect instance. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Configuration")
    void ApplyIconFromConfig();

    /** Called to cleanly end the effect, ensuring correct state for removal and analytics. */
    void Nomad_OnStatusEffectEnds();

    /** 
     * Called to trigger standard activation logic (for scripting/BP/manual triggers).
     * - Child classes should override to implement per-class startup.
     * - This enables the manager to trigger effect logic without knowing the concrete subclass.
     */
    virtual void Nomad_OnStatusEffectStarts(ACharacter* Character);

protected:
    // =====================================================
    //         ACF STATUS EFFECT OVERRIDES
    // =====================================================

    /** Called when the effect starts on a character. Handles config, sound, and init state. */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /** Called when the effect is removed from the character. Handles sound and cleanup. */
    virtual void OnStatusEffectEnds_Implementation() override;

    // =====================================================
    //         AUDIO/VISUAL HOOKS
    // =====================================================

    /** Blueprint event: called when the start sound should be played. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Status Effect | Audio/Visual")
    void OnStartSoundTriggered(USoundBase* Sound);

    /** Blueprint event: called when the end sound should be played. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Status Effect | Audio/Visual")
    void OnEndSoundTriggered(USoundBase* Sound);

    /** C++ virtual hook for additional start sound logic. */
    virtual void OnStartSoundTriggered_Implementation(USoundBase* Sound) {}

    /** C++ virtual hook for additional end sound logic. */
    virtual void OnEndSoundTriggered_Implementation(USoundBase* Sound) {}

    // =====================================================
    //         INITIALIZATION
    // =====================================================

    /** Initializes the enhanced effect. Loads config, plays sound, and sets init state. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Status Effect")
    virtual void InitializeNomadEffect();

    // =====================================================
    //         HYBRID STAT/DAMAGE APPLICATION (Override in Derived)
    // =====================================================

    /**
     * Applies this effect's main impact according to the hybrid system:
     * - StatModification: applies stat mods only.
     * - DamageEvent: applies via UE/ACF damage pipeline (requires DamageTypeClass).
     * - Both: applies both.
     * This function is to be called in OnStatusEffectStarts_Implementation, tick, etc.
     * Override in subclasses for actual logic.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Status Effect")
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& InStatMods, AActor* InTarget, UObject* InEffectConfig);

    /** Lifecycle state for safe cleanup and double-removal prevention */
    UPROPERTY()
    EEffectLifecycleState EffectState = EEffectLifecycleState::Removed;

    /** The actor responsible for causing damage (environment, enemy, item, etc.). May be nullptr for environment or self effects. */
    UPROPERTY()
    TWeakObjectPtr<AActor> DamageCauser;
    
    /** Utility: Always returns a valid actor to use as Damage Causer, never nullptr. */
    FORCEINLINE AActor* GetSafeDamageCauser(AActor* Fallback) const
    {
        return (DamageCauser.IsValid() && !DamageCauser->IsPendingKillPending()) ? DamageCauser.Get() : Fallback;
    }

private:
    
    // =====================================================
    //         INTERNAL HELPERS
    // =====================================================

    /** Loads and applies all config-driven values (tag, icon, etc). */
    void LoadConfigurationValues();

    /** Loads and plays the configured start sound at the character's location (if set). */
    void PlayStartSound();

    /** Loads and plays the configured end sound at the character's location (if set). */
    void PlayEndSound();
};