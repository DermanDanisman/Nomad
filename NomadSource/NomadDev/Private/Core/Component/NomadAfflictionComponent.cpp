// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Component/NomadAfflictionComponent.h"
#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "Core/StatusEffect/Utility/NomadStatusEffectUtils.h"
#include "Core/Debug/NomadLogCategories.h"
#include "GameFramework/Actor.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadAfflictionComponent::UNomadAfflictionComponent()
{
    // UI-only component: no ticking required
    PrimaryComponentTick.bCanEverTick = false;

    // Never replicated: UI state is local to each client
    SetIsReplicatedByDefault(false);

    // Initialize configuration
    bAutoSyncOnBeginPlay = true;
    bShowNeutralNotifications = true;

    // Initialize arrays
    ActiveAfflictions.Empty();
    EffectConfigs.Empty();

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[AFFLICTION] Component constructed"));
}

void UNomadAfflictionComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache reference to status effect manager
    if (AActor* Owner = GetOwner())
    {
        StatusEffectManager = Owner->FindComponentByClass<UNomadStatusEffectManagerComponent>();
        if (!StatusEffectManager)
        {
            UE_LOG_AFFLICTION(Warning, TEXT("[AFFLICTION] No status effect manager found on %s"),
                              *Owner->GetName());
        }
    }

    // Auto-sync with manager if enabled
    if (bAutoSyncOnBeginPlay)
    {
        SyncWithStatusEffectManager();
    }

    UE_LOG_AFFLICTION(Log, TEXT("[AFFLICTION] Component initialized"));
}

// =====================================================
//         MANAGER INTEGRATION (THE MISSING FUNCTION!)
// =====================================================

void UNomadAfflictionComponent::OnActiveEffectsChanged()
{
    // This is the function that was missing from the original implementation!
    // Called by the status effect manager when effects are added/removed/changed

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[AFFLICTION] Active effects changed, syncing UI state"));

    // Re-sync our UI state with the manager
    SyncWithStatusEffectManager();
}

void UNomadAfflictionComponent::SyncWithStatusEffectManager()
{
    // Sync our UI state with the authoritative status effect manager

    if (!StatusEffectManager)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[AFFLICTION] Cannot sync - no status effect manager"));
        return;
    }

    // Get current active effects from manager
    const TArray<FActiveEffect> ManagerEffects = StatusEffectManager->GetActiveEffects();

    // Clear our current state
    ActiveAfflictions.Empty();

    // Rebuild from manager state
    for (const FActiveEffect& Effect : ManagerEffects)
    {
        if (Effect.Tag.IsValid() && Effect.EffectInstance)
        {
            // Create notification context for this effect
            FNomadAfflictionNotificationContext Context;
            Context.AfflictionTag = Effect.Tag;
            Context.NotificationType = ENomadAfflictionNotificationType::Applied; // Default for sync
            Context.PreviousStacks = 0;
            Context.NewStacks = Effect.StackCount;

            // Get display data from config
            GetAfflictionNotificationData(
                Effect.Tag,
                ENomadAfflictionNotificationType::Applied,
                Context.DisplayName,
                Context.NotificationMessage,
                Context.NotificationColor,
                Context.NotificationDuration,
                Context.NotificationIcon
            );

            // Enhance with manager data (type, category, etc.)
            EnhanceContextWithManagerData(Context);

            ActiveAfflictions.Add(Context);
        }
    }

    // Broadcast updated state
    BroadcastStateChanges();

    UE_LOG_AFFLICTION(Log, TEXT("[AFFLICTION] Synced %d effects from manager"), ActiveAfflictions.Num());
}

// =====================================================
//         AFFLICTION STATE MANAGEMENT
// =====================================================

void UNomadAfflictionComponent::UpdateAfflictionArray(
    FGameplayTag AfflictionTag,
    ENomadAfflictionNotificationType NotificationType,
    int32 PreviousStacks,
    int32 NewStacks,
    const FText& Reason)
{
    if (!AfflictionTag.IsValid())
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[AFFLICTION] Cannot update with invalid tag"));
        return;
    }

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[AFFLICTION] Updating %s: %s (stacks %d->%d)"),
                      *AfflictionTag.ToString(),
                      *StaticEnum<ENomadAfflictionNotificationType>()->GetNameStringByValue((int64)NotificationType),
                      PreviousStacks, NewStacks);

    // Find existing affliction
    const int32 Index = FindAfflictionIndex(AfflictionTag);

    // Create notification context
    FNomadAfflictionNotificationContext Context;
    Context.AfflictionTag = AfflictionTag;
    Context.NotificationType = NotificationType;
    Context.PreviousStacks = PreviousStacks;
    Context.NewStacks = NewStacks;
    Context.Reason = Reason;

    // Get display data from config
    GetAfflictionNotificationData(
        AfflictionTag,
        NotificationType,
        Context.DisplayName,
        Context.NotificationMessage,
        Context.NotificationColor,
        Context.NotificationDuration,
        Context.NotificationIcon
    );

    // Enhance with manager data
    EnhanceContextWithManagerData(Context);

    // Handle different notification types
    switch (NotificationType)
    {
        case ENomadAfflictionNotificationType::Applied:
            {
                if (Index == INDEX_NONE)
                {
                    ActiveAfflictions.Add(Context);
                }
                else
                {
                    ActiveAfflictions[Index] = Context;
                }
                break;
            }

        case ENomadAfflictionNotificationType::Stacked:
            {
                if (Index != INDEX_NONE)
                {
                    ActiveAfflictions[Index].PreviousStacks = PreviousStacks;
                    ActiveAfflictions[Index].NewStacks = NewStacks;
                }
                else
                {
                    ActiveAfflictions.Add(Context);
                }
                break;
            }

        case ENomadAfflictionNotificationType::Unstacked:
            {
                if (Index != INDEX_NONE)
                {
                    if (NewStacks <= 0)
                    {
                        // Last stack removed
                        ActiveAfflictions.RemoveAt(Index);
                    }
                    else
                    {
                        // Update stack count
                        ActiveAfflictions[Index].PreviousStacks = PreviousStacks;
                        ActiveAfflictions[Index].NewStacks = NewStacks;
                    }
                }
                break;
            }

        case ENomadAfflictionNotificationType::Removed:
            {
                if (Index != INDEX_NONE)
                {
                    ActiveAfflictions.RemoveAt(Index);
                }
                break;
            }

        case ENomadAfflictionNotificationType::Refreshed:
            {
                if (Index != INDEX_NONE)
                {
                    ActiveAfflictions[Index] = Context;
                }
                break;
            }

        default:
            {
                // Handle other types (custom, etc.)
                if (Index != INDEX_NONE)
                {
                    ActiveAfflictions[Index] = Context;
                }
                else if (NewStacks > 0)
                {
                    ActiveAfflictions.Add(Context);
                }
                break;
            }
    }

    // Broadcast notification (for popups/toasts)
    if (bShowNeutralNotifications || Context.Category != ENomadStatusCategory::Neutral)
    {
        OnAfflictionNotification.Broadcast(Context);
    }

    // Broadcast state change (for status bars)
    BroadcastStateChanges();
}

bool UNomadAfflictionComponent::RemoveAfflictionByTag(FGameplayTag AfflictionTag)
{
    const int32 Index = FindAfflictionIndex(AfflictionTag);
    if (Index != INDEX_NONE)
    {
        ActiveAfflictions.RemoveAt(Index);
        BroadcastStateChanges();
        UE_LOG_AFFLICTION(Log, TEXT("[AFFLICTION] Removed affliction %s"), *AfflictionTag.ToString());
        return true;
    }
    return false;
}

void UNomadAfflictionComponent::ClearAllAfflictions()
{
    if (ActiveAfflictions.Num() > 0)
    {
        const int32 ClearedCount = ActiveAfflictions.Num();
        ActiveAfflictions.Empty();
        BroadcastStateChanges();
        UE_LOG_AFFLICTION(Log, TEXT("[AFFLICTION] Cleared %d afflictions"), ClearedCount);
    }
}

// =====================================================
//         UI DATA ACCESSORS
// =====================================================

TArray<FNomadAfflictionUIInfo> UNomadAfflictionComponent::GetAfflictionUIInfoArray() const
{
    TArray<FNomadAfflictionUIInfo> Result;
    Result.Reserve(ActiveAfflictions.Num());

    for (const FNomadAfflictionNotificationContext& Context : ActiveAfflictions)
    {
        Result.Add(CreateUIInfoFromContext(Context));
    }

    return Result;
}

TArray<FNomadAfflictionUIInfo> UNomadAfflictionComponent::GetAfflictionsByCategory(ENomadStatusCategory Category) const
{
    TArray<FNomadAfflictionUIInfo> Result;

    for (const FNomadAfflictionNotificationContext& Context : ActiveAfflictions)
    {
        if (Context.Category == Category)
        {
            Result.Add(CreateUIInfoFromContext(Context));
        }
    }

    return Result;
}

TArray<FNomadAfflictionUIInfo> UNomadAfflictionComponent::GetAfflictionsByType(EStatusEffectType EffectType) const
{
    TArray<FNomadAfflictionUIInfo> Result;

    for (const FNomadAfflictionNotificationContext& Context : ActiveAfflictions)
    {
        if (Context.EffectType == EffectType)
        {
            Result.Add(CreateUIInfoFromContext(Context));
        }
    }

    return Result;
}

bool UNomadAfflictionComponent::GetAfflictionInfo(FGameplayTag AfflictionTag, FNomadAfflictionUIInfo& OutInfo) const
{
    const int32 Index = FindAfflictionIndex(AfflictionTag);
    if (Index != INDEX_NONE)
    {
        OutInfo = CreateUIInfoFromContext(ActiveAfflictions[Index]);
        return true;
    }
    return false;
}

int32 UNomadAfflictionComponent::GetAfflictionCountByCategory(ENomadStatusCategory Category) const
{
    int32 Count = 0;
    for (const FNomadAfflictionNotificationContext& Context : ActiveAfflictions)
    {
        if (Context.Category == Category)
        {
            Count++;
        }
    }
    return Count;
}

// =====================================================
//         CONFIGURATION & DATA LOOKUP
// =====================================================

void UNomadAfflictionComponent::GetAfflictionNotificationData(
    FGameplayTag AfflictionTag,
    ENomadAfflictionNotificationType NotificationType,
    FText& OutDisplayName,
    FText& OutNotificationMessage,
    FLinearColor& OutColor,
    float& OutDuration,
    UTexture2D*& OutIcon) const
{
    // Look up config for this tag
    const UNomadStatusEffectConfigBase* Config = GetStatusEffectConfigForTag(AfflictionTag);

    if (Config)
    {
        // Use rich data from config asset
        OutDisplayName = Config->GetNotificationDisplayName();
        OutColor = Config->GetNotificationColor();
        OutDuration = Config->GetNotificationDuration();
        OutIcon = Config->GetNotificationIcon();

        // Get appropriate message based on notification type
        const bool bIsApplication = (NotificationType == ENomadAfflictionNotificationType::Applied ||
                                    NotificationType == ENomadAfflictionNotificationType::Stacked ||
                                    NotificationType == ENomadAfflictionNotificationType::Refreshed);
        OutNotificationMessage = Config->GetNotificationMessage(bIsApplication);
    }
    else
    {
        // Fallback data when config not found
        OutDisplayName = FText::FromName(AfflictionTag.GetTagName());
        OutColor = FLinearColor::Red;
        OutDuration = 4.0f;
        OutIcon = nullptr;

        // Generate basic message
        switch (NotificationType)
        {
            case ENomadAfflictionNotificationType::Applied:
                OutNotificationMessage = FText::Format(
                    NSLOCTEXT("Affliction", "Applied", "You are now {0}"),
                    OutDisplayName
                );
                break;
            case ENomadAfflictionNotificationType::Removed:
                OutNotificationMessage = FText::Format(
                    NSLOCTEXT("Affliction", "Removed", "You recovered from {0}"),
                    OutDisplayName
                );
                break;
            case ENomadAfflictionNotificationType::Stacked:
                OutNotificationMessage = FText::Format(
                    NSLOCTEXT("Affliction", "Stacked", "{0} intensity increased"),
                    OutDisplayName
                );
                break;
            default:
                OutNotificationMessage = FText::Format(
                    NSLOCTEXT("Affliction", "Changed", "{0} changed"),
                    OutDisplayName
                );
                break;
        }
    }
}

const UNomadStatusEffectConfigBase* UNomadAfflictionComponent::GetStatusEffectConfigForTag(FGameplayTag AfflictionTag) const
{
    // Use utility function for config lookup
    return UNomadStatusEffectUtils::FindConfigByTag(EffectConfigs, AfflictionTag);
}

// =====================================================
//         INTERNAL HELPERS
// =====================================================

int32 UNomadAfflictionComponent::FindAfflictionIndex(FGameplayTag AfflictionTag) const
{
    return ActiveAfflictions.IndexOfByPredicate([&](const FNomadAfflictionNotificationContext& Context) {
        return Context.AfflictionTag == AfflictionTag;
    });
}

FNomadAfflictionUIInfo UNomadAfflictionComponent::CreateUIInfoFromContext(const FNomadAfflictionNotificationContext& Context) const
{
    FNomadAfflictionUIInfo Info;
    Info.AfflictionTag = Context.AfflictionTag;
    Info.StackCount = Context.NewStacks;
    Info.Icon = Context.NotificationIcon;
    Info.DisplayName = Context.DisplayName;
    Info.Category = Context.Category;
    Info.EffectType = Context.EffectType;

    // Get additional info from manager if available
    if (StatusEffectManager)
    {
        Info.MaxStacks = StatusEffectManager->GetStatusEffectMaxStacks(Context.AfflictionTag);
        // Note: bCanBeManuallyRemoved would need to be added to the manager's query system
    }

    return Info;
}

void UNomadAfflictionComponent::BroadcastStateChanges()
{
    // Broadcast complete state
    const TArray<FNomadAfflictionUIInfo> UIInfo = GetAfflictionUIInfoArray();
    OnAfflictionStateChanged.Broadcast(UIInfo);

    // Broadcast category-specific updates
    for (int32 CategoryInt = 0; CategoryInt < (int32)ENomadStatusCategory::Neutral + 1; CategoryInt++)
    {
        const ENomadStatusCategory Category = (ENomadStatusCategory)CategoryInt;
        const TArray<FNomadAfflictionUIInfo> CategoryInfo = GetAfflictionsByCategory(Category);
        OnAfflictionCategoryChanged.Broadcast(Category, CategoryInfo);
    }
}

void UNomadAfflictionComponent::EnhanceContextWithManagerData(FNomadAfflictionNotificationContext& Context) const
{
    // Enhance context with data from status effect manager
    if (StatusEffectManager)
    {
        Context.EffectType = StatusEffectManager->GetStatusEffectType(Context.AfflictionTag);

        // Get category from config or fall back to manager
        const UNomadStatusEffectConfigBase* Config = GetStatusEffectConfigForTag(Context.AfflictionTag);
        if (Config)
        {
            Context.Category = Config->Category;
        }
        else
        {
            Context.Category = ENomadStatusCategory::Neutral; // Safe fallback
        }
    }
}