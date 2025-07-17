// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ANSUITypes.h"
#include "CoreMinimal.h"
#include "ANSWidgetSwitcher.h"
#include "Input/CommonUIInputSettings.h"
#include "InputCoreTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UITag.h"

#include "ANSUIPlayerSubsystem.generated.h"


class UUserWidget;
class UCommonUIInputSettings;
class UANSDeveloperSettings;
enum class EInGameMenuTabs : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFocusedWidgetChanged, UANSNavWidget*, focusedWidget);

/**
 * UANSUIPlayerSubsystem
 *
 * Manages in-game UI: spawning/removing widgets, pausing the game,
 * handling input→UI action mappings, and tab navigation.
 */
UCLASS()
class ASCENTUINAVIGATIONSYSTEM_API UANSUIPlayerSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    /**  
     * Spawns (or re-focuses) a UUserWidget of the given class.
     * @param WidgetClass    The widget blueprint/class to spawn.
     * @param bPauseGame     Whether to pause the game when showing this widget.
     * @param bLockGameInput Whether to block game input (UI-only mode) or allow both.
     * @param TabToOpen      Which tab index to activate (for widgets with a UANSWidgetSwitcher).
     * @return               The spawned (or existing) widget instance.
     */
    UFUNCTION(BlueprintCallable, Category = ANS)
    UUserWidget* SpawnInGameWidget(
        TSubclassOf<UUserWidget> WidgetClass,
        bool bPauseGame = true,
        bool bLockGameInput = true,
        EInGameMenuTabs TabToOpen = EInGameMenuTabs::Inventory
    );

    /**
     * Removes a widget from viewport and restores input/pause state if needed.
     * @param Widget          The widget instance to remove.
     * @param bUnlockUIInput  If true, switches back to GameOnly input mode.
     * @param bRemovePause    If true, unpauses the game.
     */
    UFUNCTION(BlueprintCallable, Category = ANS)
    void RemoveInGameWidget(
        UUserWidget* Widget,
        bool bUnlockUIInput = true,
        bool bRemovePause = true
    );

    /** Pops back to the previous widget on the stack. */
    UFUNCTION(BlueprintCallable, Category = ANS)
    void GoToPreviousWidget();

    /** Given a raw FKey, returns any matching UI action tags configured in CommonUIInputSettings. */
    UFUNCTION(BlueprintCallable, Category = ANS)
    bool TryGetActionsFromKey(
        const FKey& Key,
        TArray<FUIActionTag>& OutActionsTag
    );

    /** Returns all FKeys bound to the given UIActionTag in CommonUIInputSettings. */
    UFUNCTION(BlueprintCallable, Category = ANS)
    bool TryGetKeysForAction(
        const FUIActionTag& UIAction,
        TArray<FKey>& OutKeys
    );

    /** Lookup a texture icon by gameplay tag (from dev settings data table). */
    UFUNCTION(BlueprintCallable, Category = ANS)
    UTexture2D* GetIconByTag(FGameplayTag IconTag);

    /** Get display/legend config for a UIActionTag (name + icon) per input type. */
    UFUNCTION(BlueprintCallable, Category = ANS)
    bool TryGetActionConfig(
        FUIActionTag ActionName,
        const ECommonInputType& InputType,
        FANSActionConfig& OutAction
    );

    /** Convenience: fetch key icon for an action for the current input type. */
    UFUNCTION(BlueprintCallable, Category = ANS)
    UTexture2D* GetIconForUIAction(
        FUIActionTag ActionName,
        const ECommonInputType& InputType
    );

    /** Get platform-specific icon for a raw FKey. */
    UFUNCTION(BlueprintCallable, Category = ANS)
    UTexture2D* GetCurrentPlatformIconForKey(const FKey& Key) const;

    UFUNCTION(BlueprintCallable, Category = ANS)
    UUserWidget* GetCurrentWidget() const;

    /** Get icon for a raw FKey for a given platform string. */
    UFUNCTION(BlueprintCallable, Category = ANS)
    UTexture2D* GetIconForKey(
        const FKey& Key,
        const FString& Platform
    ) const;

    UFUNCTION(BlueprintCallable, Category = ANS)
    UUserWidget* HandleInGameMenuInput(TSubclassOf<UUserWidget> MenuWidgetClass, EInGameMenuTabs DesiredTab);

    /** Delegate fired when navigation focus changes within top-bar nav widgets. */
    UPROPERTY(BlueprintAssignable, Category = ANS)
    FOnFocusedWidgetChanged OnFocusChanged;

private:
    /** Helper to access the CommonUIInputSettings singleton. */
    UCommonUIInputSettings* GetInputSettings() const;

    /** Helper to access our developer settings (holds data tables, etc.). */
    UANSDeveloperSettings* GetUISettings() const;

    /** Currently active top-level widget in viewport. */
    UPROPERTY()
    TObjectPtr<UUserWidget> CurrentWidget;

    /** Stack of widget classes in spawn order, for “back” navigation. */
    UPROPERTY()
    TArray<TSubclassOf<UUserWidget>> WidgetStack;

    /** Remember default pause state so we can re-apply it on refocus. */
    bool bDefaultPauseGame = true;

    /** (Unused for now) could track mouse cursor visibility default. */
    bool bDefaultShowMouseCursor = true;
};
