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

/**
 * EEffectLifecycleState
 * ---------------------
 * Tracks the current lifecycle state of a status effect for proper cleanup and state management.
 */
UENUM(BlueprintType)
enum class EEffectLifecycleState : uint8
{
    Active,     // Effect is running normally
    Ending,     // Effect is being cleaned up
    Removed     // Effect is fully finished/cleaned up
};

// =====================================================
//        CLASS DECLARATION: BASE STATUS EFFECT
// =====================================================

/**
 * UNomadBaseStatusEffect
 * ----------------------
 * Abstract base class for all Nomad status effects.
 *
 * Key Features:
 * - Data-driven: All configuration comes from config assets
 * - Integration: Extends ACF with Nomad-specific functionality
 * - Hybrid System: Supports stat modification, damage events, or both
 * - Audio/Visual: Handles sound playback and visual effect hooks
 * - Categorization: Effects can be categorized for UI and filtering
 * - Lifecycle Management: Proper initialization and cleanup
 * - Blocking Tags: Can block certain actions (like sprinting)
 *
 * Design Philosophy:
 * - Effects are data-driven via config assets
 * - No UI logic in effects themselves (handled by manager)
 * - Blueprint-friendly with implementable events
 * - Supports damage causers for analytics and kill credit
 * - Thread-safe initialization prevents double-init
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

    // =====================================================
    //         LIFECYCLE STATE MANAGEMENT
    // =====================================================

    /** Gets the current lifecycle state of this effect. */
    UFUNCTION(BlueprintCallable, Category = "Nomad Status Effect | Getters")
    EEffectLifecycleState GetEffectLifecycleState() const { return EffectState; }

    /** Sets the lifecycle state (used internally for state transitions). */
    UFUNCTION(BlueprintCallable, Category = "Nomad Status Effect | Setters")
    void SetEffectLifecycleState(const EEffectLifecycleState NewState) { EffectState = NewState; }

    /** Sets the actor responsible for this effect (for damage attribution). */
    UFUNCTION(BlueprintCallable, Category = "Nomad Status Effect | Setters")
    void SetDamageCauser(AActor* Causer) { DamageCauser = Causer; }

    // =====================================================
    //         DATA-DRIVEN CONFIGURATION
    // =====================================================

    /**
     * The configuration asset containing all gameplay/UI parameters for this effect.
     * - Should point to a UNomadStatusEffectConfigBase (or derived class)
     * - Overrides any hardcoded properties when set
     * - Loaded synchronously at runtime
     * - Supports sharing config between multiple effect instances
     * - Determines application mode for hybrid stat/damage system
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Nomad Status Effect | Configuration")
    TSoftObjectPtr<UNomadStatusEffectConfigBase> EffectConfig;

protected:
    // =====================================================
    //         RUNTIME STATE
    // =====================================================

    /** True if this effect has been properly initialized. Prevents double-initialization. */
    UPROPERTY(BlueprintReadOnly, Category="Nomad Status Effect | Runtime")
    bool bIsInitialized = false;

    /** Current lifecycle state for safe cleanup and state tracking. */
    UPROPERTY()
    EEffectLifecycleState EffectState = EEffectLifecycleState::Removed;

    /** The actor responsible for causing this effect (may be nullptr for environmental effects). */
    UPROPERTY()
    TWeakObjectPtr<AActor> DamageCauser;

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
    //         STATUS EFFECT PROPERTIES
    // =====================================================

    /** Returns the effect's gameplay category (buff, debuff, neutral, etc.). */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Nomad Status Effect | Properties")
    ENomadStatusCategory GetStatusCategory() const;
    virtual ENomadStatusCategory GetStatusCategory_Implementation() const;

    /** Applies the gameplay tag from the config asset to this effect instance. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Configuration")
    void ApplyTagFromConfig();

    /** Applies the icon from the config asset to this effect instance. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Configuration")
    void ApplyIconFromConfig();

    // =====================================================
    //         EFFECT LIFECYCLE CONTROL
    // =====================================================

    /** Called to cleanly end the effect, ensuring proper state transitions. */
    void Nomad_OnStatusEffectEnds();

    /** Called to trigger standard activation logic (enables polymorphic activation). */
    virtual void Nomad_OnStatusEffectStarts(ACharacter* Character);

    // =====================================================
    //         BLOCKING TAG UTILITIES
    // =====================================================

    /** Applies a sprint blocking tag to prevent sprinting while this effect is active. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Blocking")
    void ApplySprintBlockTag(ACharacter* Character);

    /** Removes the sprint blocking tag when effect ends. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Blocking")
    void RemoveSprintBlockTag(ACharacter* Character);

    /** Applies a jump blocking tag to prevent jumping while this effect is active. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Blocking")
    void ApplyJumpBlockTag(ACharacter* Character);

    /** Removes the jump blocking tag when effect ends. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Blocking")
    void RemoveJumpBlockTag(ACharacter* Character);

    /** Syncs movement speed modifiers but does not apply new modifiers.
     * Use this method after modifying movement speed attributes externally.
     *
     * Previous Approach: Hardcoded RPG.Attributes.MovementSpeed
     * Current Approach: Config-driven gameplay tag approach.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Movement")
    void SyncMovementSpeedModifier(ACharacter* Character, float Multiplier);

    /** Removes movement speed modifier applied by this status effect. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Movement")
    void RemoveMovementSpeedModifier(ACharacter* Character);

    /** Syncs movement speed from configured attribute modifiers to ACF movement component. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Movement")
    static void SyncMovementSpeedFromStatusEffects(ACharacter* Character);

protected:
    // =====================================================
    //         ACF STATUS EFFECT OVERRIDES
    // =====================================================

    /** Called when the effect starts on a character. Handles config loading and initialization. */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /** Called when the effect is removed from the character. Handles cleanup and sound. */
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

    /** Initializes the Nomad effect with config loading and sound playback. */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Initialization")
    virtual void InitializeNomadEffect();

    // =====================================================
    //         HYBRID STAT/DAMAGE APPLICATION
    // =====================================================

    /**
     * Applies this effect's impact according to the hybrid system:
     * - StatModification: applies stat mods only
     * - DamageEvent: applies via UE damage pipeline
     * - Both: applies both stat mods and damage events
     *
     * Override in derived classes for specific implementation.
     *
     * @param InStatMods Array of stat modifications to apply
     * @param InTarget Target actor to apply effects to
     * @param InEffectConfig Config object containing application settings
     */
    UFUNCTION(BlueprintCallable, Category="Nomad Status Effect | Application")
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& InStatMods, AActor* InTarget, UObject* InEffectConfig);

    // =====================================================
    //         UTILITY FUNCTIONS
    // =====================================================

    /** Returns a valid actor to use as damage causer, never returns nullptr. */
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

    /** Plays the configured start sound at the character's location. */
    void PlayStartSound();

    /** Plays the configured end sound at the character's location. */
    void PlayEndSound();
};