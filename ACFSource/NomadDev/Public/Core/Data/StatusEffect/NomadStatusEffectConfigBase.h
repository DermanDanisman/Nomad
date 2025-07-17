// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ARSTypes.h"
#include "Core/StatusEffect/NomadStatusTypes.h"
#include "NomadStatusEffectConfigBase.generated.h"

class UNiagaraSystem;
class UParticleSystem;
class USoundBase;

UENUM(BlueprintType)
enum class EStatusEffectApplicationMode : uint8
{
    StatModification UMETA(DisplayName = "Stat Modification"),
    DamageEvent      UMETA(DisplayName = "Damage Event"),
    Both             UMETA(DisplayName = "Both (rare)")
};

/**
 * UNomadStatusEffectConfigBase
 * -------------------------------------------------
 * Base configuration asset for all Nomad status effects (buffs, debuffs, etc).
 * 
 * Key Features:
 * - Data-driven: All gameplay, UI, notification, and audio/visual properties are defined here.
 * - Type-agnostic: Used as the parent for instant, timed, and infinite effect configs.
 * - Integrates with all Nomad effect classes and UI/notification systems.
 * - Designer-friendly: All properties are categorized and documented for easy tuning.
 * - Validation: Robust editor- and runtime-side validation and error reporting.
 * - Hybrid stat/damage pipeline: Supports direct stat mods, DamageType, or both per effect.
 *
 * =========================================
 *      Nomad Status Effect Config (Hybrid)
 * =========================================
 *
 * This configuration asset powers all Nomad status effects (buffs, debuffs, etc).
 *
 * === Hybrid System Overview ===
 * - ApplicationMode controls how the effect applies its main gameplay impact:
 *   - StatModification: Directly modifies stats (e.g. Health, Armor, etc) via the Stat Mod arrays.
 *   - DamageEvent: Applies damage through Unreal/ACF's damage system (respects resistances, triggers damage events).
 *   - Both: Applies both (use with caution—usually not needed).
 *
 * === IMPORTANT for DamageEvent Mode ===
 * - When ApplicationMode is DamageEvent (or Both), you MUST fill out the DamageStatisticMods array:
 *     - Add at least one FStatisticValue with Statistic = Health (or appropriate stat tag), Value = damage amount (negative for damage).
 * - DamageTypeClass MUST be set (defines the type of damage, e.g. fire, poison).
 * - Stat mod arrays are ignored in DamageEvent mode (unless using Both).
 *
 * === Best Practices ===
 * - Only fill the arrays relevant for the selected ApplicationMode.
 * - Validation will warn/error if required fields are missing for the selected mode.
 * - Always use canonical GameplayTags (e.g. RPG.Statistics.Health) for stat modifications.
 * - Use negative values for damage (reduces health), positive for healing.
 *
 * See property tooltips for per-field documentation.
 */
UCLASS(Abstract, BlueprintType, meta=(DisplayName="Status Effect Config Base"))
class NOMADDEV_API UNomadStatusEffectConfigBase : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UNomadStatusEffectConfigBase();

    // ======== Basic Info ========

    /** Display name for this effect (used in UI and notifications) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Basic Info")
    FText EffectName;

    /** Description shown in tooltips or notifications */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Basic Info", meta=(MultiLine=true))
    FText Description;

    /** Icon shown in UI/notifications */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Basic Info")
    TSoftObjectPtr<UTexture2D> Icon;

    /** Unique tag for this effect (required for stacking/removal and logic) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Basic Info")
    FGameplayTag EffectTag;

    /** Category (Positive/Negative/Neutral) for UI, filtering, and logic */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Basic Info")
    ENomadStatusCategory Category = ENomadStatusCategory::Neutral;

    // ======== Hybrid System: Damage/Stat Mod Application ========

    /** 
     * How this effect should apply its main impact (stat modification, damage event, or both).
     * StatModification: Direct stat changes (legacy/current system).
     * DamageEvent: Uses DamageType and UE/ACF pipeline (futureproof, supports resistances/immunities).
     * Both: Applies both; use with caution to avoid double-counting.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Application Mode")
    EStatusEffectApplicationMode ApplicationMode = EStatusEffectApplicationMode::StatModification;

    /** 
     * (Optional) DamageType to use for DamageEvent or Both modes.
     * If null, will not trigger UE/ACF damage pipeline.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Application Mode", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::StatModification", EditConditionHides))
    TSubclassOf<UDamageType> DamageTypeClass;

    /**
     * Damage Stat Mods (for DamageEvent/Both modes)
     *
     * Used only when ApplicationMode is DamageEvent or Both.
     * - Add one or more FStatisticValue entries with Statistic = Health (or similar), Value = amount to apply (negative for damage).
     * - The value is passed to UGameplayStatics::ApplyDamage.
     * - If empty, no damage will be applied and validation will fail.
     *
     * Example:
     *   - Statistic: RPG.Statistics.Health
     *   - Value: -50.f // deals 50 damage
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Application Mode | Stats", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::StatModification", EditConditionHides))
    TArray<FStatisticValue> DamageStatisticMods;
    

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hybrid Control")
    bool bPlayHitReaction = true; // If false, suppress hit reaction

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hybrid Control")
    bool bEnableMotionWarp = true; // If false, don't do motion warping

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hybrid Control")
    bool bCustomDamageCalculation = false; // If true, use effect's custom damage logic

    // ======== Notification UI Overrides (optional) ========

    /** Color for notification popups (optional; transparent = auto-color by category) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification", meta=(HideAlphaChannel))
    FLinearColor NotificationColor = FLinearColor::Transparent;

    /** Duration to show notification (optional, in seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification", meta=(ClampMin="0.1", UIMin="0.1"))
    float NotificationDuration = 4.f;

    /** Custom message when effect is applied (optional) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification")
    FText AppliedMessage;

    /** Custom message when effect is removed (optional) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notification")
    FText RemovedMessage;

    // ======== Behavior ========

    /** Should show notifications for this effect? (UI popups, etc) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behavior")
    bool bShowNotifications = true;

    /** Can this effect stack with itself? (multiple applications) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behavior")
    bool bCanStack = false;

    /** Maximum stacks if stacking is enabled (must be >= 1) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behavior", meta=(EditCondition="bCanStack", ClampMin="1"))
    int32 MaxStacks = 1;

    // ======== Audio/Visual ========

    /** Sound played when effect starts (optional) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio/Visual")
    TSoftObjectPtr<USoundBase> StartSound;

    /** Sound played when effect ends (optional) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio/Visual")
    TSoftObjectPtr<USoundBase> EndSound;

    /** Particle effect to attach to character (optional, legacy) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio/Visual")
    TSoftObjectPtr<UParticleSystem> AttachedEffect;

    /** Niagara effect to attach to character (optional, modern) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio/Visual")
    TSoftObjectPtr<UNiagaraSystem> AttachedNiagaraEffect;

    // ======== Notification UI Helper Functions ========

    /** Returns the notification icon for this effect (loads or returns nullptr) */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    UTexture2D* GetNotificationIcon() const
    {
        return Icon.IsValid() ? Icon.Get() : nullptr;
    }

    /** Returns the display name for this effect (for UI popups/tooltips) */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    FText GetNotificationDisplayName() const
    {
        return EffectName;
    }

    /** Returns the description for this effect (tooltips, etc) */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    FText GetNotificationDescription() const
    {
        return Description;
    }

    /** Returns the notification color, or falls back to category color if unset */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    FLinearColor GetNotificationColor() const
    {
        // If a color is set, use it; otherwise, use category color.
        return NotificationColor.A > 0 ? NotificationColor : (Category == ENomadStatusCategory::Negative ? FLinearColor::Red : FLinearColor::Green);
    }

    /** Returns the notification duration (seconds, default 4.0) */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    float GetNotificationDuration() const
    {
        return NotificationDuration > 0.f ? NotificationDuration : 4.f;
    }

    /** Returns the notification message for applied/removed (with fallback text) */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    FText GetNotificationMessage(bool bWasAdded) const
    {
        if (bWasAdded && !AppliedMessage.IsEmpty())
            return AppliedMessage;
        if (!bWasAdded && !RemovedMessage.IsEmpty())
            return RemovedMessage;
        return bWasAdded
            ? FText::Format(NSLOCTEXT("StatusEffect", "Applied", "You are now {0}!"), EffectName)
            : FText::Format(NSLOCTEXT("StatusEffect", "Removed", "You recovered from {0}."), EffectName);
    }

    // ======== Utility Functions ========

    /** Returns a brief type description for UI/debug (overridden in subclasses) */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    virtual FText GetEffectTypeDescription() const { return FText::FromString(TEXT("Base Effect")); }

    /** Runtime or editor validation: is this config valid for use? */
    UFUNCTION(BlueprintCallable, Category="Status Effect Config")
    virtual bool IsConfigValid() const;

    /** Returns all validation errors (used by editor, logs, etc) */
    UFUNCTION(BlueprintCallable, Category="Status Effect Config")
    virtual TArray<FString> GetValidationErrors() const;

protected:
#if WITH_EDITOR
    /** Editor-only validation: respond to property changes for safety and auto-correction */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};