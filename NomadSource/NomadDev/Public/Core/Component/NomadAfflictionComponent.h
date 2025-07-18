// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Core/StatusEffect/NomadStatusTypes.h"
#include "NomadAfflictionComponent.generated.h"

class UNomadStatusEffectConfigBase;
class UNomadStatusEffectManagerComponent;

/**
 * FNomadAfflictionNotificationContext
 * -----------------------------------
 * Complete context for an affliction notification event.
 * Used by UI to display popups/toasts and drive detailed feedback.
 * Contains all relevant info about the change, including before/after stack count, icon, color, and message.
 */
USTRUCT(BlueprintType)
struct NOMADDEV_API FNomadAfflictionNotificationContext
{
    GENERATED_BODY()

    /** GameplayTag identifying the specific affliction/status effect */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    FGameplayTag AfflictionTag;

    /** The type of notification event */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    ENomadAfflictionNotificationType NotificationType;

    /** Display name for UI, from config or fallback to tag name */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    FText DisplayName;

    /** Main notification message for UI popups */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    FText NotificationMessage;

    /** Color for UI notification */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    FLinearColor NotificationColor = FLinearColor::Red;

    /** How long to display the notification (seconds) */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    float NotificationDuration = 4.0f;

    /** Icon to display in UI */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    UTexture2D* NotificationIcon = nullptr;

    /** Previous stack count (before change) */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    int32 PreviousStacks = 0;

    /** New stack count (after change) */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    int32 NewStacks = 0;

    /** Optional reason for notification */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    FText Reason;

    /** Effect category for filtering and display */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    ENomadStatusCategory Category = ENomadStatusCategory::Neutral;

    /** Effect type for UI behavior */
    UPROPERTY(BlueprintReadOnly, Category="Affliction")
    EStatusEffectType EffectType = EStatusEffectType::Unknown;

    /** Default constructor */
    FNomadAfflictionNotificationContext()
    {
        AfflictionTag = FGameplayTag();
        NotificationType = ENomadAfflictionNotificationType::Applied;
        DisplayName = FText();
        NotificationMessage = FText();
        NotificationColor = FLinearColor::Red;
        NotificationDuration = 4.0f;
        NotificationIcon = nullptr;
        PreviousStacks = 0;
        NewStacks = 0;
        Reason = FText();
        Category = ENomadStatusCategory::Neutral;
        EffectType = EStatusEffectType::Unknown;
    }
};

/**
 * FNomadAfflictionUIInfo
 * ----------------------
 * Lightweight struct for UI widgets (icon bars, tooltips, etc.)
 * Contains only essential display information.
 */
USTRUCT(BlueprintType)
struct NOMADDEV_API FNomadAfflictionUIInfo
{
    GENERATED_BODY()

    /** Tag for the affliction/status effect */
    UPROPERTY(BlueprintReadOnly, Category="UI")
    FGameplayTag AfflictionTag;

    /** Number of stacks of this affliction */
    UPROPERTY(BlueprintReadOnly, Category="UI")
    int32 StackCount = 1;

    /** Icon to display */
    UPROPERTY(BlueprintReadOnly, Category="UI")
    UTexture2D* Icon = nullptr;

    /** Display name for the affliction */
    UPROPERTY(BlueprintReadOnly, Category="UI")
    FText DisplayName;

    /** Effect category for color coding */
    UPROPERTY(BlueprintReadOnly, Category="UI")
    ENomadStatusCategory Category = ENomadStatusCategory::Neutral;

    /** Effect type for UI behavior */
    UPROPERTY(BlueprintReadOnly, Category="UI")
    EStatusEffectType EffectType = EStatusEffectType::Unknown;

    /** Maximum stacks possible (for progress bars) */
    UPROPERTY(BlueprintReadOnly, Category="UI")
    int32 MaxStacks = 1;

    /** Whether this effect can be manually removed */
    UPROPERTY(BlueprintReadOnly, Category="UI")
    bool bCanBeManuallyRemoved = false;
};

// =====================================================
//                    DELEGATES
// =====================================================

/** Delegate for broadcasting notifications to UI widgets */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FAfflictionNotificationDelegate,
    const FNomadAfflictionNotificationContext&, NotificationContext
);

/** Delegate for broadcasting the complete affliction state to UI */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FAfflictionStateDelegate,
    const TArray<FNomadAfflictionUIInfo>&, AfflictionState
);

/** Delegate for category-specific updates (buffs/debuffs separately) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FAfflictionCategoryDelegate,
    ENomadStatusCategory, Category,
    const TArray<FNomadAfflictionUIInfo>&, CategoryAfflictions
);

// =====================================================
//              CLASS DECLARATION
// =====================================================

/**
 * UNomadAfflictionComponent
 * -------------------------
 * Enhanced UI-facing component for tracking and broadcasting status effect changes.
 *
 * Key Features:
 * - Maintains active affliction state for UI display
 * - Provides rich notification context for popups/toasts
 * - Supports category-based filtering (buffs vs debuffs)
 * - Integrates seamlessly with the status effect manager
 * - Handles config lookup for UI data (icons, colors, names)
 * - Optimized for UI performance with lightweight structs
 *
 * Design Philosophy:
 * - Pure UI component - no gameplay logic
 * - Event-driven updates from status effect manager
 * - Rich context for sophisticated UI feedback
 * - Supports both immediate notifications and persistent state
 * - Designer-friendly with config asset integration
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOMADDEV_API UNomadAfflictionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // =====================================================
    //         CONSTRUCTOR & INITIALIZATION
    // =====================================================

    /** Constructor - sets up UI-only component */
    UNomadAfflictionComponent();

    // =====================================================
    //         MANAGER INTEGRATION (MISSING FUNCTION)
    // =====================================================

    /**
     * Called by status effect manager when active effects change.
     * This was the missing function that caused the error!
     */
    UFUNCTION(BlueprintCallable, Category="Affliction|Manager Integration")
    void OnActiveEffectsChanged();

    /**
     * Syncs with the status effect manager to rebuild UI state.
     * Called during initialization and when needed.
     */
    UFUNCTION(BlueprintCallable, Category="Affliction|Manager Integration")
    void SyncWithStatusEffectManager();

    // =====================================================
    //         AFFLICTION STATE MANAGEMENT
    // =====================================================

    /**
     * Updates affliction state and broadcasts notifications.
     * Called by the status effect manager for all status changes.
     *
     * @param AfflictionTag      Tag identifying the affliction
     * @param NotificationType   Type of change (applied, removed, stacked, etc.)
     * @param PreviousStacks     Stack count before change
     * @param NewStacks          Stack count after change
     * @param Reason             Optional reason text for detailed feedback
     */
    UFUNCTION(BlueprintCallable, Category="Affliction|State")
    void UpdateAfflictionArray(
        FGameplayTag AfflictionTag,
        ENomadAfflictionNotificationType NotificationType,
        int32 PreviousStacks = 0,
        int32 NewStacks = 0,
        const FText& Reason = FText()
    );

    /**
     * Removes an affliction by tag (used for cleanup).
     * Returns true if the affliction was found and removed.
     */
    UFUNCTION(BlueprintCallable, Category="Affliction|State")
    bool RemoveAfflictionByTag(FGameplayTag AfflictionTag);

    /**
     * Clears all afflictions (used on respawn, level change, etc.)
     */
    UFUNCTION(BlueprintCallable, Category="Affliction|State")
    void ClearAllAfflictions();

    // =====================================================
    //         UI DATA ACCESSORS
    // =====================================================

    /** Returns lightweight UI info for all active afflictions */
    UFUNCTION(BlueprintPure, Category="Affliction|UI")
    TArray<FNomadAfflictionUIInfo> GetAfflictionUIInfoArray() const;

    /** Returns UI info filtered by category (buffs, debuffs, neutral) */
    UFUNCTION(BlueprintPure, Category="Affliction|UI")
    TArray<FNomadAfflictionUIInfo> GetAfflictionsByCategory(ENomadStatusCategory Category) const;

    /** Returns UI info filtered by effect type (timed, infinite, etc.) */
    UFUNCTION(BlueprintPure, Category="Affliction|UI")
    TArray<FNomadAfflictionUIInfo> GetAfflictionsByType(EStatusEffectType EffectType) const;

    /** Gets specific affliction info by tag */
    UFUNCTION(BlueprintPure, Category="Affliction|UI")
    bool GetAfflictionInfo(FGameplayTag AfflictionTag, FNomadAfflictionUIInfo& OutInfo) const;

    /** Returns count of active afflictions by category */
    UFUNCTION(BlueprintPure, Category="Affliction|UI")
    int32 GetAfflictionCountByCategory(ENomadStatusCategory Category) const;

    // =====================================================
    //         CONFIGURATION & DATA LOOKUP
    // =====================================================

    /**
     * Looks up notification data from config assets.
     * Provides all UI information needed for rich notifications.
     */
    UFUNCTION(BlueprintCallable, Category="Affliction|Config")
    void GetAfflictionNotificationData(
        FGameplayTag AfflictionTag,
        ENomadAfflictionNotificationType NotificationType,
        FText& OutDisplayName,
        FText& OutNotificationMessage,
        FLinearColor& OutColor,
        float& OutDuration,
        UTexture2D*& OutIcon
    ) const;

    /** Finds config asset for a given tag */
    UFUNCTION(BlueprintPure, Category="Affliction|Config")
    const UNomadStatusEffectConfigBase* GetStatusEffectConfigForTag(FGameplayTag AfflictionTag) const;

    // =====================================================
    //         EVENT DELEGATES
    // =====================================================

    /** Broadcasts individual notifications (for popups/toasts) */
    UPROPERTY(BlueprintAssignable, Category="Affliction|Events")
    FAfflictionNotificationDelegate OnAfflictionNotification;

    /** Broadcasts complete affliction state (for status bars) */
    UPROPERTY(BlueprintAssignable, Category="Affliction|Events")
    FAfflictionStateDelegate OnAfflictionStateChanged;

    /** Broadcasts category-specific updates (buffs vs debuffs) */
    UPROPERTY(BlueprintAssignable, Category="Affliction|Events")
    FAfflictionCategoryDelegate OnAfflictionCategoryChanged;

    // =====================================================
    //         CONFIGURATION
    // =====================================================

    /** Array of all effect configs for tag lookup (set by designer) */
    UPROPERTY(EditDefaultsOnly, Category="Affliction|Configuration", meta=(
        ToolTip="Array of all status effect configs for UI data lookup"))
    TArray<TObjectPtr<UNomadStatusEffectConfigBase>> EffectConfigs;

    /** Whether to automatically sync with manager on BeginPlay */
    UPROPERTY(EditAnywhere, Category="Affliction|Configuration", meta=(
        ToolTip="Automatically sync with status effect manager when component starts"))
    bool bAutoSyncOnBeginPlay = true;

    /** Whether to show notifications for neutral effects */
    UPROPERTY(EditAnywhere, Category="Affliction|Configuration", meta=(
        ToolTip="Show UI notifications for neutral category effects"))
    bool bShowNeutralNotifications = true;

protected:
    // =====================================================
    //         COMPONENT LIFECYCLE
    // =====================================================

    virtual void BeginPlay() override;

    // =====================================================
    //         INTERNAL STATE
    // =====================================================

    /** Current active afflictions with full context */
    UPROPERTY(BlueprintReadOnly, Category="Affliction|State")
    TArray<FNomadAfflictionNotificationContext> ActiveAfflictions;

    /** Cached reference to status effect manager */
    UPROPERTY()
    TObjectPtr<UNomadStatusEffectManagerComponent> StatusEffectManager;

private:
    // =====================================================
    //         INTERNAL HELPERS
    // =====================================================

    /** Finds active affliction index by tag */
    int32 FindAfflictionIndex(FGameplayTag AfflictionTag) const;

    /** Creates UI info from notification context */
    FNomadAfflictionUIInfo CreateUIInfoFromContext(const FNomadAfflictionNotificationContext& Context) const;

    /** Broadcasts state change events */
    void BroadcastStateChanges();

    /** Gets enhanced info from status effect manager */
    void EnhanceContextWithManagerData(FNomadAfflictionNotificationContext& Context) const;
};