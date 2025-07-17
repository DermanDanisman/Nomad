// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ANSWidgetSwitcher.h"
#include "ANSUIPlayerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "UITag.h"
#include <Engine/GameInstance.h>
#include <Components/HorizontalBox.h>
#include "ANSNavWidget.h"

void UANSWidgetSwitcher::ProcessOnKeyDown(const FKeyEvent& InKeyEvent)
{
    // Grab our UI subsystem (handles key → action tag mapping)
    if (UANSUIPlayerSubsystem* UISub = GetUISubsystem())
    {
        TArray<FUIActionTag> UIactions;
        // Try translating the pressed key to one or more UI action tags
        if (UISub->TryGetActionsFromKey(InKeyEvent.GetKey(), UIactions))
        {
            // If the “next” action is present, move forward
            if (UIactions.Contains(NextAction))
            {
                NavigateToNext();
            }
            // Else if the “previous” action is present, move back
            else if (UIactions.Contains(PreviousAction))
            {
                NavigateToPrevious();
            }
        }
    }
}

void UANSWidgetSwitcher::NavigateToNext()
{
    const int32 CurrentIndex = GetActiveWidgetIndex();
    const int32 NextIndex = CurrentIndex + 1;

    // If there's a real next widget, show it…
    if (NextIndex < GetNumWidgets())
    {
        SetActiveWidgetIndex(NextIndex);
    }
    // …otherwise wrap if allowed
    else if (bAllowCircularNavigation)
    {
        SetActiveWidgetIndex(0);
    }
}

void UANSWidgetSwitcher::NavigateToPrevious()
{
    const int32 CurrentIndex = GetActiveWidgetIndex();
    const int32 PrevIndex = CurrentIndex - 1;

    // If valid, show previous
    if (PrevIndex >= 0)
    {
        SetActiveWidgetIndex(PrevIndex);
    }
    // Otherwise wrap to last
    else if (bAllowCircularNavigation)
    {
        SetActiveWidgetIndex(GetNumWidgets() - 1);
    }
}

UWidget* UANSWidgetSwitcher::GetCurrentActiveWidget() const
{
    // Return the widget at our current index
    return GetWidgetAtIndex(GetActiveWidgetIndex());
}

void UANSWidgetSwitcher::SetTopBar(UHorizontalBox* topbar)
{
    // Cache the top bar so we can focus corresponding nav button later
    Topbar = topbar;
}


void UANSWidgetSwitcher::SetActiveWidgetIndex(int32 Index)
{
    // Call parent to actually swap widgets
    Super::SetActiveWidgetIndex(Index);

    // If we've bound a Topbar, focus its child at the same index
    if (Topbar && Topbar->GetChildrenCount() > Index)
    {
        if (UANSNavWidget* widgetRef = Cast<UANSNavWidget>(Topbar->GetChildAt(Index)))
        {
            widgetRef->SetKeyboardFocus();
        }
    }
}

void UANSWidgetSwitcher::SwitchToTab(EInGameMenuTabs Tab)
{
    int32 TabIndex = static_cast<int32>(Tab);
    if (TabIndex >= 0 && TabIndex < GetNumWidgets())
    {
        SetActiveWidgetIndex(TabIndex);
        OnTabChanged.Broadcast(Tab);
    }
}

void UANSWidgetSwitcher::HandleSlateActiveIndexChanged(int32 ActiveIndex)
{
    // Let the base class update visuals/animations
    Super::HandleSlateActiveIndexChanged(ActiveIndex);

    // Clamp to valid range
    const int32 Clamped = FMath::Clamp(ActiveIndex, 0, GetNumWidgets() - 1);

    // Cast to our enum type
    const EInGameMenuTabs NewTab = static_cast<EInGameMenuTabs>(Clamped);
    CurrentTab = NewTab;  // update our field

    // Fire the enum-based delegate so listeners know which tab is active
    OnTabChanged.Broadcast(NewTab);
}

UANSUIPlayerSubsystem* UANSWidgetSwitcher::GetUISubsystem() const
{
    // Pull game instance, then get our custom subsystem
    if (const UGameInstance* gameInst = UGameplayStatics::GetGameInstance(this))
    {
        return gameInst->GetSubsystem<UANSUIPlayerSubsystem>();
    }
    return nullptr;
}
