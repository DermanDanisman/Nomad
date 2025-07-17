// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NomadAfflictionComponent.generated.h"

class UNomadStatusEffectConfigBase;

/**
 * Enum: ENomadAfflictionNotificationType
 * --------------------------------------
 * Describes the type of affliction notification event.
 * Used to communicate the reason or context for an affliction state change.
 * This allows UI and systems to react appropriately (e.g. play a different sound for "Removed" vs "Expired").
 */
UENUM(BlueprintType)
enum class ENomadAfflictionNotificationType : uint8
{
    Applied         UMETA(DisplayName="Applied"),        // Affliction was newly applied
    Refreshed       UMETA(DisplayName="Refreshed"),      // Duration or effect was refreshed
    Stacked         UMETA(DisplayName="Stacked"),        // Additional stack was added
    Unstacked       UMETA(DisplayName="Unstacked"),      // Stack was removed
    Removed         UMETA(DisplayName="Removed"),        // Affliction was manually removed (e.g. by cleanse)
    Expired         UMETA(DisplayName="Expired"),        // Affliction expired naturally (duration elapsed)
    Cleansed        UMETA(DisplayName="Cleansed"),       // Affliction was removed by a cleansing effect
    Immune          UMETA(DisplayName="Immune"),         // Application failed due to immunity
    Overwritten     UMETA(DisplayName="Overwritten"),    // Affliction was replaced/overwritten by another
    Custom          UMETA(DisplayName="Custom")          // Custom/unspecified change
};

/**
 * Struct: FNomadAfflictionNotificationContext
 * -------------------------------------------
 * Provides complete context for an affliction notification event.
 * Used by UI to display popups/toasts and to drive detailed feedback.
 * Contains all relevant info about the change, including before/after stack count, icon, color, and a message.
 */
USTRUCT(BlueprintType)
struct FNomadAfflictionNotificationContext {
    GENERATED_BODY()

    /** GameplayTag identifying the specific affliction/status effect. Used as the unique key. */
    UPROPERTY(BlueprintReadOnly)
    FGameplayTag AfflictionTag;

    /** The type of notification (see ENomadAfflictionNotificationType). */
    UPROPERTY(BlueprintReadOnly)
    ENomadAfflictionNotificationType NotificationType;

    /** Display name for UI, from config or fallback to tag name. */
    UPROPERTY(BlueprintReadOnly)
    FText DisplayName;

    /** Main notification message for UI popups, based on event. */
    UPROPERTY(BlueprintReadOnly)
    FText NotificationMessage;

    /** Color for UI notification (e.g. red = debuff, green = cleanse). */
    UPROPERTY(BlueprintReadOnly)
    FLinearColor NotificationColor = FLinearColor::Red;

    /** How long to display the notification (in seconds). */
    UPROPERTY(BlueprintReadOnly)
    float NotificationDuration = 4.f;

    /** Icon to display in UI, from config or fallback. */
    UPROPERTY(BlueprintReadOnly)
    UTexture2D* NotificationIcon = nullptr;

    /** Previous stack count (before change). Useful for stack up/down events. */
    UPROPERTY(BlueprintReadOnly)
    int32 PreviousStacks = 0;

    /** New stack count (after change). */
    UPROPERTY(BlueprintReadOnly)
    int32 NewStacks = 0;

    /** Optional: Reason for notification (e.g. "Cleansed by potion"). */
    UPROPERTY(BlueprintReadOnly)
    FText Reason;

    /** Default constructor, initializes members to safe defaults. */
    FNomadAfflictionNotificationContext()
        : AfflictionTag()
        , NotificationType(ENomadAfflictionNotificationType::Custom)
        , DisplayName()
        , NotificationMessage()
        , NotificationColor(FLinearColor::Red)
        , NotificationDuration(4.f)
        , NotificationIcon(nullptr)
        , PreviousStacks(0)
        , NewStacks(0)
        , Reason()
    {}
};

/**
 * Struct: FNomadAfflictionUIInfo
 * ------------------------------
 * Simple struct for summarizing affliction for UI widgets (icon, name, stack count).
 * Used for UI affliction bars, tooltips, etc; does not include notification data.
 */
USTRUCT(BlueprintType)
struct FNomadAfflictionUIInfo
{
    GENERATED_BODY()

    /** Tag for the affliction/status effect (unique identifier). */
    UPROPERTY(BlueprintReadOnly)
    FGameplayTag AfflictionTag;

    /** Number of stacks of this affliction (1 if not stackable). */
    UPROPERTY(BlueprintReadOnly)
    int32 StackCount = 1;

    /** Icon to display (nullable). */
    UPROPERTY(BlueprintReadOnly)
    UTexture2D* Icon = nullptr;

    /** Display name for the affliction (for UI display). */
    UPROPERTY(BlueprintReadOnly)
    FText DisplayName;
};

/** Delegate for broadcasting the full array of active afflictions to the UI.
 *  Widgets should bind to this and update their displays when notified.
 *  If bIsRemovalNotification is true, show removal popup only, do NOT update affliction bar.
 *  If false, update the bar/list with new state.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FAfflictionArrayNotification,
    const TArray<FNomadAfflictionNotificationContext>&, AfflictionContexts
);

/**
 * UNomadAfflictionComponent
 * -------------------------------------------------------
 * UI-facing component for tracking and broadcasting all active afflictions/status effects.
 * Responsibilities:
 * - Maintains an array of active afflictions, including stack counts and rich metadata.
 * - Looks up config assets for notification data (icon, color, name, etc).
 * - Broadcasts all changes to UI via OnAfflictionArrayNotification.
 * - Pure UI: no replication or core gameplay logic, only frontend state.
 * - All updates should go through UpdateAfflictionArray().
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOMADDEV_API UNomadAfflictionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /** Standard constructor. Disables ticking and replication since this is UI-only. */
    UNomadAfflictionComponent();

    /**
     * Updates the array of afflictions and broadcasts the new state.
     * - Handles all affliction changes (apply, stack, remove, cleanse, etc.).
     * - Notifies the UI with the new context and rich data.
     * - Always call this for affliction changes; never update ActiveAfflictions directly.
     *
     * @param AfflictionTag      The gameplay tag identifying the affliction.
     * @param NotificationType   What kind of change this is (applied, stacked, removed, etc).
     * @param PreviousStacks     Stack count before the change.
     * @param NewStacks          Stack count after the change.
     * @param Reason             Optional text explaining the reason (for detailed feedback).
     */
    UFUNCTION(BlueprintCallable, Category="Affliction")
    void UpdateAfflictionArray(
        FGameplayTag AfflictionTag,
        ENomadAfflictionNotificationType NotificationType,
        int32 PreviousStacks = 0,
        int32 NewStacks = 0,
        const FText& Reason = FText()
    );

    /**
     * Looks up the config for the affliction and fills out all UI notification data.
     * - Returns icon, color, name, message, and duration, based on event type.
     * - If no config is found, uses generic fallback values.
     *
     * @param AfflictionTag            The gameplay tag for the effect.
     * @param NotificationType         What kind of UI event (applied, removed, etc.).
     * @param OutDisplayName           [out] UI display name (from config or tag).
     * @param OutNotificationMessage   [out] UI message (from config or fallback).
     * @param OutColor                 [out] Message color (config or fallback).
     * @param OutDuration              [out] Display duration (config or fallback).
     * @param OutIcon                  [out] Icon pointer (config or nullptr).
     */
    UFUNCTION(BlueprintCallable, Category="Affliction")
    void GetAfflictionNotificationData(
        FGameplayTag AfflictionTag,
        ENomadAfflictionNotificationType NotificationType,
        FText& OutDisplayName,
        FText& OutNotificationMessage,
        FLinearColor& OutColor,
        float& OutDuration,
        UTexture2D*& OutIcon
    ) const;
    
    /** Broadcasts current affliction array to UI. Widgets should bind to this for real-time updates. */
    UPROPERTY(BlueprintAssignable, Category="Affliction|Notifications")
    FAfflictionArrayNotification OnAfflictionArrayNotification;

    /** Array of all effect configs to search by tag. Set in editor (designer must keep up to date). */
    UPROPERTY(EditDefaultsOnly, Category="Affliction|Config")
    TArray<TObjectPtr<UNomadStatusEffectConfigBase>> EffectConfigs;

    /**
     * Returns the status effect config asset for a given gameplay tag.
     * - If not found, returns nullptr and fallback logic will be used.
     * - Used for all UI lookups (icon, name, color, etc).
     */
    const UNomadStatusEffectConfigBase* GetStatusEffectConfigForTag(FGameplayTag AfflictionTag) const;

    /** The current array of active afflictions (with rich metadata). Used by UI. */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    TArray<FNomadAfflictionNotificationContext> ActiveAfflictions;

    /**
     * Returns a UI-friendly summary array (icon, name, stack count) for widgets.
     * - Intended for icon bars, tooltips, etc.
     * - More lightweight than full notification context.
     */
    UFUNCTION(BlueprintPure, Category="Affliction|UI")
    TArray<FNomadAfflictionUIInfo> GetAfflictionUIInfoArray() const;
};