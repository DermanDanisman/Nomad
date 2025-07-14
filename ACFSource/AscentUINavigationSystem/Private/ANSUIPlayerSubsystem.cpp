// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ANSUIPlayerSubsystem.h"
#include "ANSDeveloperSettings.h"
#include "ANSUITypes.h"
#include "ANSWidgetSwitcher.h"
#include "Blueprint/UserWidget.h"
#include "CommonActionWidget.h"
#include "Engine/DataTable.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIInputSettings.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "UITag.h"
#include <CommonButtonBase.h>

UUserWidget* UANSUIPlayerSubsystem::SpawnInGameWidget(
    const TSubclassOf<UUserWidget> WidgetClass,
    const bool bPauseGame,
    const bool bLockGameInput,
    const EInGameMenuTabs TabToOpen
)
{
    // Fetch player controller (0 = first local player).
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController) {
        return nullptr;  // cannot spawn without a PC
    }

    // If same widget already up, just switch its tab rather than re-spawn.
    if (CurrentWidget && CurrentWidget->GetClass() == WidgetClass) {
        if (UANSWidgetSwitcher* Switcher =
                Cast<UANSWidgetSwitcher>(CurrentWidget->GetWidgetFromName(TEXT("MenuTabsSwitcher"))))
        {
            Switcher->SwitchToTab(TabToOpen);
        }
        return CurrentWidget;
    }

    // Create and add the widget to viewport.
    UUserWidget* SpawnedWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
    if (SpawnedWidget) {
        CurrentWidget = SpawnedWidget;
        SpawnedWidget->AddToViewport();

        // Pause/unpause
        bDefaultPauseGame = bPauseGame;
        UGameplayStatics::SetGamePaused(PlayerController, bPauseGame);

        // Input mode: UI only vs. Game+UI
        if (bLockGameInput) {
            FInputModeUIOnly UIOnly;
            PlayerController->SetInputMode(UIOnly);
            UIOnly.SetWidgetToFocus(SpawnedWidget->GetCachedWidget());
        }
        else
        {
            FInputModeGameAndUI GameAndUI;
            PlayerController->SetInputMode(GameAndUI);
            GameAndUI.SetWidgetToFocus(SpawnedWidget->GetCachedWidget());
        }

        // Stop pawn movement to prevent character sliding while UI is open.
        if (APawn* Pawn = PlayerController->GetPawn()) {
            if (Pawn->GetMovementComponent()) {
                Pawn->GetMovementComponent()->StopMovementImmediately();
            }
        }

        // Maintain back-stack: remove duplicates then push.
        WidgetStack.Remove(WidgetClass);
        WidgetStack.Add(WidgetClass);

        // If this widget has a named switcher, select the requested tab.
        if (UANSWidgetSwitcher* Switcher =
                Cast<UANSWidgetSwitcher>(SpawnedWidget->GetWidgetFromName(TEXT("MenuTabsSwitcher"))))
        {
            Switcher->SwitchToTab(TabToOpen);
        }
    }

    return SpawnedWidget;
}

void UANSUIPlayerSubsystem::RemoveInGameWidget(
    UUserWidget* Widget,
    const bool bUnlockUIInput,
    const bool bRemovePause
) {
    // Remove from viewport and pop from our stack.
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!Widget || !PlayerController) {
        return;
    }

    Widget->RemoveFromParent();
    WidgetStack.Remove(Widget->GetClass());

    // Only reset pause/input if this was the active widget.
    if (Widget == CurrentWidget) {
        CurrentWidget = nullptr;

        if (bRemovePause) {
            UGameplayStatics::SetGamePaused(PlayerController, false);
        }
        if (bUnlockUIInput) {
            FInputModeGameOnly GameOnly;
            PlayerController->SetInputMode(GameOnly);
        } else {
            FInputModeUIOnly UIOnly;
            PlayerController->SetInputMode(UIOnly);
        }
    }
}

UUserWidget* UANSUIPlayerSubsystem::GetCurrentWidget() const
{
    return CurrentWidget;
}

void UANSUIPlayerSubsystem::GoToPreviousWidget() {
    // Peek the last class in stack (current), remove it, then respawn the next one.
    if (WidgetStack.Num() == 0) {
        return;
    }
    TSubclassOf<UUserWidget> PrevClass = WidgetStack.Last();

    // If it's the same as current, pop it off first.
    if (CurrentWidget && CurrentWidget->GetClass() == PrevClass) {
        RemoveInGameWidget(CurrentWidget, true, false);
        WidgetStack.Pop();
    }

    // Spawn the next one down, if any.
    if (WidgetStack.Num() > 0) {
        PrevClass = WidgetStack.Last();
        WidgetStack.Pop();
        SpawnInGameWidget(PrevClass, UGameplayStatics::IsGamePaused(this));
    }
}

bool UANSUIPlayerSubsystem::TryGetActionsFromKey(
    const FKey& Key,
    TArray<FUIActionTag>& OutActionsTag
) {
    const UCommonUIInputSettings* Settings = GetInputSettings();
    for (const FUIInputAction& Action : Settings->GetUIInputActions()) {
        for (const FUIActionKeyMapping& Map : Action.KeyMappings) {
            if (Map.Key == Key) {
                OutActionsTag.Add(Action.ActionTag);
            }
        }
    }
    return OutActionsTag.Num() > 0;
}

bool UANSUIPlayerSubsystem::TryGetKeysForAction(
    const FUIActionTag& UIAction,
    TArray<FKey>& OutKeys
) {
    const UCommonUIInputSettings* Settings = GetInputSettings();
    for (const FUIInputAction& Action : Settings->GetUIInputActions()) {
        if (Action.ActionTag == UIAction) {
            OutKeys.Empty();
            for (const auto& Map : Action.KeyMappings) {
                OutKeys.Add(Map.Key);
            }
            return true;
        }
    }
    return false;
}

UCommonUIInputSettings* UANSUIPlayerSubsystem::GetInputSettings() const {
    // Common UI input settings lives in Default objects.
    return GetMutableDefault<UCommonUIInputSettings>();
}

UANSDeveloperSettings* UANSUIPlayerSubsystem::GetUISettings() const {
    // Developer-exposed settings for data tables / icons.
    return GetMutableDefault<UANSDeveloperSettings>();
}

UTexture2D* UANSUIPlayerSubsystem::GetIconByTag(const FGameplayTag IconTag) {
    const UDataTable* Table = GetUISettings()->GetIconsByTagDT();
    if (!Table) {
        UE_LOG(LogTemp, Error, TEXT("IconsByTagDT not set in UANSDeveloperSettings"));
        return nullptr;
    }
    // Iterate each row looking for matching tag.
    for (auto& Pair : Table->GetRowMap()) {
        if (const FANSIcons* Row = (FANSIcons*)Pair.Value) {
            if (Row->IconTag == IconTag) {
                return Row->Icon;
            }
        }
    }
    return nullptr;
}

bool UANSUIPlayerSubsystem::TryGetActionConfig(
    FUIActionTag ActionName,
    const ECommonInputType& InputType,
    FANSActionConfig& OutAction
) {
    const UCommonUIInputSettings* Settings = GetInputSettings();
    // Find the action object
    const FUIInputAction* Action = Settings->GetUIInputActions()
        .FindByPredicate([ActionName](const FUIInputAction& A) {
            return A.ActionTag == ActionName;
        });
    if (!Action) {
        UE_LOG(LogTemp, Error, TEXT("ActionTag not found: %s"), *ActionName.ToString());
        return false;
    }
    OutAction.Action = ActionName;
    OutAction.UIName = Action->DefaultDisplayName;

    // Pick the right key/icon for current input method
    for (auto& Map : Action->KeyMappings) {
        bool bMatchGamepad = InputType == ECommonInputType::Gamepad && Map.Key.IsGamepadKey();
        bool bMatchKBM     = InputType == ECommonInputType::MouseAndKeyboard && !Map.Key.IsGamepadKey();
        if (bMatchGamepad || bMatchKBM) {
            OutAction.KeyIcon = GetCurrentPlatformIconForKey(Map.Key);
            return true;
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("No matching key mapping for action %s"), *ActionName.ToString());
    return false;
}

UTexture2D* UANSUIPlayerSubsystem::GetIconForUIAction(
    const FUIActionTag ActionName,
    const ECommonInputType& InputType
) {
    FANSActionConfig Config;
    if (TryGetActionConfig(ActionName, InputType, Config)) {
        return Config.KeyIcon;
    }
    return nullptr;
}

UTexture2D* UANSUIPlayerSubsystem::GetCurrentPlatformIconForKey(const FKey& Key) const {
    FString Platform = UGameplayStatics::GetPlatformName();
    return GetIconForKey(Key, Platform);
}

UTexture2D* UANSUIPlayerSubsystem::GetIconForKey(const FKey& Key, const FString& Platform) const {
    const UDataTable* Table = GetUISettings()->GetKeysConfigByPlatformDT(Platform);
    if (!Table) {
        UE_LOG(LogTemp, Error, TEXT("PlatformIconsDT not set for platform %s"), *Platform);
        return nullptr;
    }
    for (auto& Pair : Table->GetRowMap()) {
        if (const FANSKeysIconConfig* Row = (FANSKeysIconConfig*)Pair.Value) {
            if (Row->Key == Key) {
                return Row->KeyIcon;
            }
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("No icon found for key %s on platform %s"), *Key.GetFName().ToString(), *Platform);
    return nullptr;
}

UUserWidget* UANSUIPlayerSubsystem::HandleInGameMenuInput(const TSubclassOf<UUserWidget> MenuWidgetClass, EInGameMenuTabs DesiredTab)
{
    if (CurrentWidget)
    {
        if (UANSWidgetSwitcher* Switcher = Cast<UANSWidgetSwitcher>(CurrentWidget->GetWidgetFromName(TEXT("MenuTabsSwitcher"))))
        {
            if (Switcher->GetCurrentTab() == DesiredTab)
            {
                // Same tab pressed again, close the menu and RETURN immediately
                RemoveInGameWidget(CurrentWidget, true, true);
                return nullptr;  // <-- Important! Prevents reopening
            }
            else
            {
                // Different tab pressed, just switch inside
                Switcher->SwitchToTab(DesiredTab);
                return CurrentWidget;
            }
        }
    }

    // Menu is not open, open it
    UUserWidget* Widget = SpawnInGameWidget(MenuWidgetClass, false, true, DesiredTab);
    CurrentWidget = Widget;
    return CurrentWidget;
}