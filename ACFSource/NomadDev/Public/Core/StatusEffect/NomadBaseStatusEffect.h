// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StatusEffects/ACFBaseStatusEffect.h"
#include "Core/StatusEffect/NomadStatusTypes.h"
#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundBase.h"
#include "NomadBaseStatusEffect.generated.h"

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
 *
 * === Hybrid System ===
 * - All status effect logic (stat mods, damage, or both) is data-driven by the config asset.
 * - DO NOT implement direct stat/damage application here—always call ApplyHybridEffect with values from config.
 * - Use GetBaseConfig() to access all gameplay and UI parameters.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta=(DisplayName="Nomad Base Status Effect"))
class NOMADDEV_API UNomadBaseStatusEffect : public UACFBaseStatusEffect
{
    GENERATED_BODY()

public:
    /** Default constructor: initializes runtime state. */
    UNomadBaseStatusEffect();

    /** Lifecycle state for safe cleanup and double-removal prevention */
    UPROPERTY(BlueprintReadOnly, Category="Lifecycle")
    EEffectLifecycleState EffectState = EEffectLifecycleState::Removed;

    // --- NEW: Damage Causer support ---
    /** The actor responsible for causing damage (environment, enemy, item, etc). May be nullptr for environment or self effects. */
    UPROPERTY(BlueprintReadWrite, Category="Status Effect")
    TWeakObjectPtr<AActor> DamageCauser;

protected:
    // ======== Data-Driven Configuration ========

    /**
     * The configuration asset containing all gameplay/UI parameters for this effect.
     * - Should point to a UNomadStatusEffectConfigBase (BP or C++).
     * - If set, overrides any hardcoded properties.
     * - Loaded synchronously at runtime if not already loaded.
     * - Allows multiple effect instances to share config.
     * - **Hybrid:** Determines application mode for stat/damage/both.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Configuration")
    TSoftObjectPtr<UNomadStatusEffectConfigBase> BaseConfig;

    // ======== Runtime State ========

    /**
     * True if this effect has already been initialized.
     * Prevents accidental double-initialization.
     * Set to true after InitializeNomadEffect() succeeds, reset on end.
     */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    bool bIsInitialized = false;

public:
    // ======== Configuration Access and Application ========

    /**
     * Loads and returns the config asset for this effect, or nullptr if not set.
     * - Loads synchronously at runtime.
     * - Use this to access all gameplay/UI parameters for this effect.
     */
    UFUNCTION(BlueprintPure, Category="Configuration")
    UNomadStatusEffectConfigBase* GetBaseConfig() const;

    /**
     * Applies all configuration values from the config asset to this effect instance.
     * - Sets tags, icons, and any other config-driven properties.
     * - Call this during initialization.
     * - Will log and early-out if config is missing or invalid.
     */
    UFUNCTION(BlueprintCallable, Category="Configuration")
    virtual void ApplyBaseConfiguration();

    /**
     * Returns true if the base configuration is set and passes internal validation.
     * - Use this before relying on config-driven properties.
     */
    UFUNCTION(BlueprintPure, Category="Configuration")
    bool HasValidBaseConfiguration() const;

    // ======== Status Effect Properties (Category, Tag, Icon) ========

    /**
     * Returns the effect's gameplay category (debuff, buff, neutral, etc).
     * - Virtual: can be overridden by subclasses for dynamic logic.
     * - Default: reads from config asset's Category property.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Status Effect")
    ENomadStatusCategory GetStatusCategory() const;
    virtual ENomadStatusCategory GetStatusCategory_Implementation() const;

    /**
     * Applies the gameplay tag from the config asset to this effect instance.
     * - Uses ACF's SetStatusEffectTag.
     * - Tag is used for stacking and removal, NOT display (use config for UI).
     */
    UFUNCTION(BlueprintCallable, Category="Configuration")
    void ApplyTagFromConfig();

    /**
     * Applies the icon from the config asset to this effect instance.
     * - Uses ACF's SetStatusIcon.
     * - Icon is used for UI but is not responsible for notification popups.
     */
    UFUNCTION(BlueprintCallable, Category="Configuration")
    void ApplyIconFromConfig();

    /**
     * Called when the effect starts on a character.
     * - Calls parent implementation.
     * - Applies config, plays start sound, and sets bIsInitialized.
     * - Subclasses can override for custom logic, but should call Super.
     */
    void Nomad_OnStatusEffectEnds();

protected:
    // ======== ACF Status Effect Overrides ========

    /**
     * Called when the effect starts on a character.
     * - Calls parent implementation.
     * - Applies config, plays start sound, and sets bIsInitialized.
     * - Subclasses can override for custom logic, but should call Super.
     */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /**
     * Called when the effect is removed from the character.
     * - Plays end sound, resets bIsInitialized, and calls parent implementation.
     */
    virtual void OnStatusEffectEnds_Implementation() override;

    // ======== Audio/Visual Hooks ========

    /**
     * Blueprint event: called when the start sound should be played.
     * - Use this for visual FX or additional cues.
     * - Sound will have already been played by C++.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Audio/Visual")
    void OnStartSoundTriggered(USoundBase* Sound);

    /**
     * Blueprint event: called when the end sound should be played.
     * - Use this for visual FX or cleanup.
     * - Sound will have already been played by C++.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Audio/Visual")
    void OnEndSoundTriggered(USoundBase* Sound);

    /**
     * C++ virtual hook for additional start sound logic.
     * - Called after C++ sound playback, before BP event.
     */
    virtual void OnStartSoundTriggered_Implementation(USoundBase* Sound) {}

    /**
     * C++ virtual hook for additional end sound logic.
     * - Called after C++ sound playback, before BP event.
     */
    virtual void OnEndSoundTriggered_Implementation(USoundBase* Sound) {}

    // ======== Initialization ========

    /**
     * Initializes the enhanced effect.
     * - Loads and applies config.
     * - Plays start sound.
     * - Sets bIsInitialized.
     * - Logs errors and skips if already initialized or missing owner.
     * - Call this at the start of your effect logic.
     */
    UFUNCTION(BlueprintCallable, Category="Status Effect")
    virtual void InitializeNomadEffect();

    // ======== Hybrid Stat/Damage Application (to be overridden in derived classes) ========
    /**
     * Applies this effect's main impact according to the hybrid system:
     * - StatModification: applies stat mods only.
     * - DamageEvent: applies via UE/ACF damage pipeline (requires DamageTypeClass).
     * - Both: applies both.
     * This function is to be called in OnStatusEffectStarts_Implementation, tick, etc.
     * Override in subclasses for actual logic.
     */
    UFUNCTION(BlueprintCallable, Category="Status Effect")
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfig);

    /** Utility: Always returns a valid actor to use as Damage Causer, never nullptr. */
    FORCEINLINE AActor* GetValidCauser(TWeakObjectPtr<AActor> InDamageCauser, AActor* Fallback) const
    {
        return (InDamageCauser.IsValid() && !InDamageCauser->IsPendingKillPending()) ? InDamageCauser.Get() : Fallback;
    }

private:
    // ======== Internal Helpers ========

    /**
     * Loads and applies all config-driven values (tag, icon, etc).
     * - Called by ApplyBaseConfiguration and InitializeNomadEffect.
     */
    void LoadConfigurationValues();

    /**
     * Loads and plays the configured start sound at the character's location (if set).
     * - Triggers both C++ and BP events.
     */
    void PlayStartSound();

    /**
     * Loads and plays the configured end sound at the character's location (if set).
     * - Triggers both C++ and BP events.
     */
    void PlayEndSound();
};