// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Component/NomadAfflictionComponent.h"
#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "Core/StatusEffect/Utility/NomadStatusEffectUtils.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadAfflictionComponent::UNomadAfflictionComponent()
{
    // This component is strictly UI/UX: no ticking required.
    PrimaryComponentTick.bCanEverTick = false;

    // Never replicated: UI state is only relevant to local client.
    SetIsReplicatedByDefault(false);
}

// =====================================================
//         AFFLICTION STATE UPDATE & NOTIFICATIONS
// =====================================================

void UNomadAfflictionComponent::UpdateAfflictionArray(
    const FGameplayTag AfflictionTag,
    const ENomadAfflictionNotificationType NotificationType,
    const int32 PreviousStacks,
    const int32 NewStacks,
    const FText& Reason
)
{
    // Find index of affliction in the current active array.
    const int32 Index = ActiveAfflictions.IndexOfByPredicate([&](const FNomadAfflictionNotificationContext& Ctx) {
        return Ctx.AfflictionTag == AfflictionTag;
    });

    // Compose notification context for this event.
    FNomadAfflictionNotificationContext Context;
    Context.AfflictionTag = AfflictionTag;
    Context.NotificationType = NotificationType;
    Context.PreviousStacks = PreviousStacks;
    Context.NewStacks = NewStacks;
    Context.Reason = Reason;

    // Look up all display data (icon, color, name, duration, message).
    GetAfflictionNotificationData(
        AfflictionTag,
        NotificationType,
        Context.DisplayName,
        Context.NotificationMessage,
        Context.NotificationColor,
        Context.NotificationDuration,
        Context.NotificationIcon
    );

    // --------- Handle stack change events robustly ------------

    // If last stack is lost (via Unstacked or Removed), treat as full removal for UI.
    if ((NotificationType == ENomadAfflictionNotificationType::Unstacked || NotificationType == ENomadAfflictionNotificationType::Removed)
        && NewStacks <= 0)
    {
        if (Index != INDEX_NONE)
        {
            // Notify UI with "unstacked/removed" popup for last stack lost.
            TArray<FNomadAfflictionNotificationContext> SingleRemoval;
            SingleRemoval.Add(Context);
            OnAfflictionArrayNotification.Broadcast(SingleRemoval);

            // Remove and notify state sync.
            ActiveAfflictions.RemoveAt(Index);
            OnAfflictionArrayNotification.Broadcast(ActiveAfflictions);
        }
        return;
    }

    // Stacking up or down, but stacks remain.
    if (NotificationType == ENomadAfflictionNotificationType::Stacked ||
        NotificationType == ENomadAfflictionNotificationType::Unstacked)
    {
        if (Index == INDEX_NONE)
        {
            // New entry (shouldn't happen for Unstacked, but defensive).
            ActiveAfflictions.Add(Context);
        }
        else
        {
            // Update stack counts and display details.
            ActiveAfflictions[Index].PreviousStacks = PreviousStacks;
            ActiveAfflictions[Index].NewStacks = NewStacks;
            ActiveAfflictions[Index].NotificationIcon = Context.NotificationIcon;
            ActiveAfflictions[Index].DisplayName = Context.DisplayName;
        }
        // Always notify UI with stack up/down popup.
        TArray<FNomadAfflictionNotificationContext> StackChange;
        StackChange.Add(Context);
        OnAfflictionArrayNotification.Broadcast(StackChange);

        // And broadcast full state for affliction bar.
        OnAfflictionArrayNotification.Broadcast(ActiveAfflictions);
        return;
    }

    // Removal (manual, cleanse, expired) with stacks left (rare, but possible).
    if ((NotificationType == ENomadAfflictionNotificationType::Removed ||
         NotificationType == ENomadAfflictionNotificationType::Cleansed ||
         NotificationType == ENomadAfflictionNotificationType::Expired)
        && NewStacks > 0)
    {
        if (Index != INDEX_NONE)
        {
            ActiveAfflictions[Index].PreviousStacks = PreviousStacks;
            ActiveAfflictions[Index].NewStacks = NewStacks;
            // UI: show popup for removed, but keep in bar.
            TArray<FNomadAfflictionNotificationContext> Removal;
            Removal.Add(Context);
            OnAfflictionArrayNotification.Broadcast(Removal);
            OnAfflictionArrayNotification.Broadcast(ActiveAfflictions);
        }
        return;
    }

    // Application, refresh, or custom: add or update as usual.
    if (NotificationType == ENomadAfflictionNotificationType::Applied ||
        NotificationType == ENomadAfflictionNotificationType::Refreshed ||
        NotificationType == ENomadAfflictionNotificationType::Custom)
    {
        if (Index == INDEX_NONE)
            ActiveAfflictions.Add(Context);
        else
            ActiveAfflictions[Index] = Context;

        // Popup for event.
        TArray<FNomadAfflictionNotificationContext> Notif;
        Notif.Add(Context);
        OnAfflictionArrayNotification.Broadcast(Notif);
        // And state sync.
        OnAfflictionArrayNotification.Broadcast(ActiveAfflictions);
        return;
    }

    // Fallback: always broadcast state.
    OnAfflictionArrayNotification.Broadcast(ActiveAfflictions);
}

bool UNomadAfflictionComponent::RemoveAfflictionByTag(FGameplayTag AfflictionTag)
{
    const int32 Index = ActiveAfflictions.IndexOfByPredicate([&](const FNomadAfflictionNotificationContext& Ctx) {
        return Ctx.AfflictionTag == AfflictionTag;
    });
    if (Index != INDEX_NONE)
    {
        ActiveAfflictions.RemoveAt(Index);
        OnAfflictionArrayNotification.Broadcast(ActiveAfflictions);
        return true;
    }
    return false;
}

// =====================================================
//         CONFIGURATION & DATA LOOKUP
// =====================================================

void UNomadAfflictionComponent::GetAfflictionNotificationData(
    const FGameplayTag AfflictionTag,
    const ENomadAfflictionNotificationType NotificationType,
    FText& OutDisplayName,
    FText& OutNotificationMessage,
    FLinearColor& OutColor,
    float& OutDuration,
    UTexture2D*& OutIcon
) const
{
    // Look up the config asset for this tag (designer must keep EffectConfigs up to date!).
    const UNomadStatusEffectConfigBase* Config = GetStatusEffectConfigForTag(AfflictionTag);

    if (Config)
    {
        // Use all rich data from the config asset (designer-driven).
        OutDisplayName = Config->GetNotificationDisplayName();
        OutColor = Config->GetNotificationColor();
        OutDuration = Config->GetNotificationDuration();
        OutIcon = Config->GetNotificationIcon();
        // Choose message variant based on event type (e.g. removed/expired gets different message).
        OutNotificationMessage = Config->GetNotificationMessage(NotificationType == ENomadAfflictionNotificationType::Removed ? false : true);
    }
    else
    {
        // Fallback: Use tag name, generic color/message if config not found.
        OutDisplayName = FText::FromName(AfflictionTag.GetTagName());
        OutColor = FLinearColor::Red;
        OutDuration = 4.f;
        OutIcon = nullptr;
        switch (NotificationType)
        {
            case ENomadAfflictionNotificationType::Applied:
                OutNotificationMessage = NSLOCTEXT("Affliction", "AfflictionApplied", "You are now afflicted!");
                break;
            case ENomadAfflictionNotificationType::Removed:
                OutNotificationMessage = NSLOCTEXT("Affliction", "AfflictionRemoved", "Affliction removed.");
                break;
            default:
                OutNotificationMessage = NSLOCTEXT("Affliction", "AfflictionChanged", "Affliction changed.");
                break;
        }
    }
}

const UNomadStatusEffectConfigBase* UNomadAfflictionComponent::GetStatusEffectConfigForTag(const FGameplayTag AfflictionTag) const
{
    // Use utility function for DRY code; null if not found.
    return UNomadStatusEffectUtils::FindConfigByTag(EffectConfigs, AfflictionTag);
}

// =====================================================
//         UI DATA ACCESSORS
// =====================================================

TArray<FNomadAfflictionUIInfo> UNomadAfflictionComponent::GetAfflictionUIInfoArray() const
{
    // Build a lightweight UI array for widgets (icon, name, stack count).
    TArray<FNomadAfflictionUIInfo> Result;
    for (const FNomadAfflictionNotificationContext& Ctx : ActiveAfflictions)
    {
        FNomadAfflictionUIInfo Info;
        Info.AfflictionTag = Ctx.AfflictionTag;
        Info.StackCount = Ctx.NewStacks;
        Info.Icon = Ctx.NotificationIcon;
        Info.DisplayName = Ctx.DisplayName;
        Result.Add(Info);
    }
    return Result;
}