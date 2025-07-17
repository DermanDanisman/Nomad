// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Component/NomadAfflictionComponent.h"
#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "Core/StatusEffect/Utility/NomadStatusEffectUtils.h"

/**
 * UNomadAfflictionComponent
 * -------------------------------------------------------
 * Implementation of UI-facing affliction tracking and notification broadcast.
 * See header for detailed documentation.
 */

UNomadAfflictionComponent::UNomadAfflictionComponent()
{
    // This component is strictly UI/UX: no ticking required.
    PrimaryComponentTick.bCanEverTick = false;

    // Never replicated: UI state is only relevant to local client.
    SetIsReplicatedByDefault(false);
}

void UNomadAfflictionComponent::UpdateAfflictionArray(
    FGameplayTag AfflictionTag,
    ENomadAfflictionNotificationType NotificationType,
    int32 PreviousStacks,
    int32 NewStacks,
    const FText& Reason
)
{
    // Find index of affliction in the current active array.
    int32 Index = ActiveAfflictions.IndexOfByPredicate([&](const FNomadAfflictionNotificationContext& Ctx) {
        return Ctx.AfflictionTag == AfflictionTag;
    });

    // Compose a rich context struct for notification/UI.
    FNomadAfflictionNotificationContext Context;
    Context.AfflictionTag = AfflictionTag;
    Context.NotificationType = NotificationType;
    Context.PreviousStacks = PreviousStacks;
    Context.NewStacks = NewStacks;
    Context.Reason = Reason;

    // Lookup all display data (icon, color, name, duration, message).
    GetAfflictionNotificationData(
        AfflictionTag,
        NotificationType,
        Context.DisplayName,
        Context.NotificationMessage,
        Context.NotificationColor,
        Context.NotificationDuration,
        Context.NotificationIcon
    );

    // Handle removals: for Removed, Expired, or Cleansed, we simply remove the entry.
    if (NotificationType == ENomadAfflictionNotificationType::Removed ||
        NotificationType == ENomadAfflictionNotificationType::Expired ||
        NotificationType == ENomadAfflictionNotificationType::Cleansed)
    {
        if (Index != INDEX_NONE)
        {
            // Broadcast removal notification as a one-item array (so UI can show "removed" message even after removal)
            TArray<FNomadAfflictionNotificationContext> SingleRemoval;
            SingleRemoval.Add(Context);
            OnAfflictionArrayNotification.Broadcast(SingleRemoval);

            // Now remove from active array and broadcast the updated array (for UI state sync)
            ActiveAfflictions.RemoveAt(Index);
            OnAfflictionArrayNotification.Broadcast(ActiveAfflictions);
        }
        
    }
    // For application, stacks, refresh, or custom, we add or update the entry.
    else
    {
        // Add to array if not present, or update existing entry.
        if (Index == INDEX_NONE)
            ActiveAfflictions.Add(Context);
        else
            ActiveAfflictions[Index] = Context;
    }

    // Always broadcast the full updated array to UI (widgets should listen for this).
    OnAfflictionArrayNotification.Broadcast(ActiveAfflictions);
}

void UNomadAfflictionComponent::GetAfflictionNotificationData(
    FGameplayTag AfflictionTag,
    ENomadAfflictionNotificationType NotificationType,
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

const UNomadStatusEffectConfigBase* UNomadAfflictionComponent::GetStatusEffectConfigForTag(FGameplayTag AfflictionTag) const
{
    // Use utility function for DRY code; null if not found.
    return UNomadStatusEffectUtils::FindConfigByTag(EffectConfigs, AfflictionTag);
}

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