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

/**
 * UNomadStatusEffectConfigBase
 * ----------------------------
 * Base configuration asset for all Nomad status effects.
 *
 * Key Features:
 * - Data-driven: All gameplay, UI, and audio/visual properties
 * - Type-agnostic: Parent for instant, timed, and infinite configs
 * - Hybrid System: Supports stat modification, damage events, or both
 * - Validation: Robust editor and runtime validation
 * - Designer-friendly: Categorized and documented properties
 *
 * Design Philosophy:
 * - All values come from config assets, not hardcoded
 * - Supports both Blueprint and C++ workflows
 * - Comprehensive validation prevents common mistakes
 * - Future-proof with hybrid application modes
 */
UCLASS(Abstract, BlueprintType, meta=(DisplayName="Status Effect Config Base"))
class NOMADDEV_API UNomadStatusEffectConfigBase : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UNomadStatusEffectConfigBase();

    // =====================================================
    //         BASIC INFORMATION
    // =====================================================

    /** Display name for this effect (used in UI and notifications) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Basic Info", meta=(
        ToolTip="The name shown to players in UI elements and notifications"))
    FText EffectName;

    /** Description shown in tooltips or notifications */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Basic Info", meta=(
        MultiLine=true, ToolTip="Detailed description for tooltips and help text"))
    FText Description;

    /** Icon shown in UI/notifications */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Basic Info", meta=(
        ToolTip="Icon displayed in status bars and notifications"))
    TSoftObjectPtr<UTexture2D> Icon;

    /** Unique tag for this effect (required for stacking/removal and logic) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Basic Info", meta=(
        ToolTip="Unique gameplay tag that identifies this effect"))
    FGameplayTag EffectTag;

    /** Category (Positive/Negative/Neutral) for UI, filtering, and logic */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Basic Info", meta=(
        ToolTip="Category determines UI color coding and effect grouping"))
    ENomadStatusCategory Category = ENomadStatusCategory::Neutral;

    // =====================================================
    //         HYBRID APPLICATION SYSTEM
    // =====================================================

    /**
     * How this effect applies its main impact.
     * - StatModification: Direct stat changes (fast, simple)
     * - DamageEvent: Uses UE damage pipeline (supports resistances, events)
     * - Both: Applies both (use carefully to avoid double-counting)
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Application Mode", meta=(
        ToolTip="Determines how the effect applies damage/healing/stat changes"))
    EStatusEffectApplicationMode ApplicationMode = EStatusEffectApplicationMode::StatModification;

    /**
     * DamageType for DamageEvent or Both modes.
     * Required when using damage events for proper resistance/immunity handling.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Application Mode", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::StatModification",
        EditConditionHides, ToolTip="DamageType class for UE damage pipeline integration"))
    TSubclassOf<UDamageType> DamageTypeClass;

    /**
     * Damage/healing values for DamageEvent/Both modes.
     * Use negative values for damage, positive for healing.
     * Only affects Health stat in damage mode.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Application Mode", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::StatModification",
        EditConditionHides, ToolTip="Stat modifications applied via damage events"))
    TArray<FStatisticValue> DamageStatisticMods;

    /** Use custom damage calculation instead of standard damage pipeline */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Application Mode", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::StatModification",
        ToolTip="Enable for effects that need special damage calculation logic"))
    bool bCustomDamageCalculation = false;

    // =====================================================
    //         BEHAVIOR SETTINGS
    // =====================================================

    /** Should show notifications for this effect? (UI popups, etc) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behavior", meta=(
        ToolTip="Whether to show UI notifications when effect is applied/removed"))
    bool bShowNotifications = true;

    /** Can this effect stack with itself? (multiple applications) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behavior", meta=(
        ToolTip="Allow multiple instances of this effect on the same target"))
    bool bCanStack = false;

    /** Maximum stacks if stacking is enabled (must be >= 1) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behavior", meta=(
        EditCondition="bCanStack", ClampMin="1",
        ToolTip="Maximum number of stacks allowed"))
    int32 MaxStackSize = 1;

    // =====================================================
    //         BLOCKING SYSTEM
    // =====================================================

    /** Tags that this effect blocks (e.g. Status.Block.Sprint, Status.Block.Jump) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blocking", meta=(
        ToolTip="Gameplay tags for actions this effect prevents"))
    FGameplayTagContainer BlockingTags;

    // =====================================================
    //         AUDIO/VISUAL EFFECTS
    // =====================================================

    /** Sound played when effect starts (optional) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio/Visual", meta=(
        ToolTip="Audio cue played when effect begins"))
    TSoftObjectPtr<USoundBase> StartSound;

    /** Sound played when effect ends (optional) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio/Visual", meta=(
        ToolTip="Audio cue played when effect ends"))
    TSoftObjectPtr<USoundBase> EndSound;

    /** Legacy particle effect (use Niagara for new effects) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio/Visual", meta=(
        ToolTip="Legacy Cascade particle system (deprecated, use Niagara)"))
    TSoftObjectPtr<UParticleSystem> AttachedEffect;

    /** Modern Niagara effect to attach to character */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio/Visual", meta=(
        ToolTip="Niagara effect system attached to character"))
    TSoftObjectPtr<UNiagaraSystem> AttachedNiagaraEffect;

    // =====================================================
    //         NOTIFICATION CUSTOMIZATION
    // =====================================================

    /** Color for notification popups (transparent = auto-color by category) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notifications", meta=(
        HideAlphaChannel, ToolTip="Custom color for notifications (leave transparent for auto-color)"))
    FLinearColor NotificationColor = FLinearColor::Transparent;

    /** Duration to show notification (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notifications", meta=(
        ClampMin="0.1", UIMin="0.1", ToolTip="How long notifications are displayed"))
    float NotificationDuration = 4.0f;

    /** Custom message when effect is applied (optional) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notifications", meta=(
        ToolTip="Custom text shown when effect is applied"))
    FText AppliedMessage;

    /** Custom message when effect is removed (optional) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Notifications", meta=(
        ToolTip="Custom text shown when effect is removed"))
    FText RemovedMessage;

    // =====================================================
    //         UTILITY FUNCTIONS
    // =====================================================

    /** Returns the notification icon for this effect */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    UTexture2D* GetNotificationIcon() const;

    /** Returns the display name for this effect */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    FText GetNotificationDisplayName() const { return EffectName; }

    /** Returns the description for this effect */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    FText GetNotificationDescription() const { return Description; }

    /** Returns the notification color with fallback to category color */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    FLinearColor GetNotificationColor() const;

    /** Returns the notification duration */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    float GetNotificationDuration() const { return FMath::Max(0.1f, NotificationDuration); }

    /** Returns the notification message with fallbacks */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    FText GetNotificationMessage(bool bWasAdded) const;

    /** Returns a brief type description (overridden in subclasses) */
    UFUNCTION(BlueprintPure, Category="Status Effect Config")
    virtual FText GetEffectTypeDescription() const { return FText::FromString(TEXT("Base Effect")); }

    /** Runtime validation: is this config valid for use? */
    UFUNCTION(BlueprintCallable, Category="Status Effect Config")
    virtual bool IsConfigValid() const;

    /** Returns all validation errors */
    UFUNCTION(BlueprintCallable, Category="Status Effect Config")
    virtual TArray<FString> GetValidationErrors() const;

protected:
#if WITH_EDITOR
    /** Editor validation and auto-correction */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};