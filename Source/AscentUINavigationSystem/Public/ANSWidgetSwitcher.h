// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CommonAnimatedSwitcher.h"          // Base class for animated widget switchers
#include "Components/WidgetSwitcher.h"       // Underlying widget switcher support
#include "CoreMinimal.h"                     // Core UE types and macros
#include "UITag.h"                           // Definitions for FUIActionTag
#include "ANSWidgetSwitcher.generated.h"     // Generated UHT code

// Strongly-typed enum for our in-game tabs
UENUM(BlueprintType)
enum class EInGameMenuTabs : uint8
{
    Inventory    UMETA(DisplayName="Inventory"),  // 0
    Quest        UMETA(DisplayName="Quest"),      // 1
    Status       UMETA(DisplayName="Status"),     // 2
    Map          UMETA(DisplayName="Map"),        // 3
};

// Delegate that fires when the active tab changes, passing the new enum value
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FANSOnTabChanged, EInGameMenuTabs, NewTab);

struct FUIActionTag;                   // forward: action mapping
class UANSUIPlayerSubsystem;           // forward: subsystem for input
class UHorizontalBox;                  // forward: UI container

UCLASS()
class ASCENTUINAVIGATIONSYSTEM_API UANSWidgetSwitcher : public UCommonAnimatedSwitcher
{
    GENERATED_BODY()

public:
    /** Handle raw key events for menu navigation */
    UFUNCTION(BlueprintCallable, Category = ANS)
    void ProcessOnKeyDown(const FKeyEvent& InKeyEvent);

    /** Move forward one tab */
    UFUNCTION(BlueprintCallable, Category = ANS)
    void NavigateToNext();

    /** Move backward one tab */
    UFUNCTION(BlueprintCallable, Category = ANS)
    void NavigateToPrevious();

    /** Bind a horizontal box containing tab buttons */
    UFUNCTION(BlueprintCallable, Category = ANS)
    void SetTopBar(UHorizontalBox* topbar);

    /** Event: broadcast when the active tab changes (enum) */
    UPROPERTY(BlueprintAssignable, Category = ANS)
    FANSOnTabChanged OnTabChanged;

    /** Get the currently shown child widget */
    UFUNCTION(BlueprintPure, BlueprintCallable, Category = ANS)
    UWidget* GetCurrentActiveWidget() const;

    /** Override: switch by integer index */
    virtual void SetActiveWidgetIndex(int32 Index) override;

    /** Convenience: switch by enum directly */
    UFUNCTION(BlueprintCallable, Category = ANS)
    void SetActiveTab(EInGameMenuTabs NewTab)
    {
        // Cast enum to its underlying int and call the index-based setter
        SetActiveWidgetIndex(static_cast<int32>(NewTab));
    }

    UFUNCTION(BlueprintCallable)
    void SwitchToTab(EInGameMenuTabs Tab);

    UFUNCTION(BlueprintCallable)
    EInGameMenuTabs GetCurrentTab() const { return CurrentTab; }

protected:
    /** If true, wrapping past last/first cycles back around */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ANS)
    bool bAllowCircularNavigation = true;

    /** Input action for “previous tab” */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ANS)
    FUIActionTag PreviousAction;

    /** Input action for “next tab” */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ANS)
    FUIActionTag NextAction;

    /** Intercept underlying index change, translate to enum + broadcast */
    virtual void HandleSlateActiveIndexChanged(int32 ActiveIndex) override;

    /** Optional top-bar container for button focus */
    UPROPERTY(BlueprintReadOnly, Category = ANS)
    UHorizontalBox* Topbar;

    /** Remember the last active tab as an enum */
    UPROPERTY(BlueprintReadOnly, Category = ANS)
    EInGameMenuTabs CurrentTab = EInGameMenuTabs::Inventory;

private:
    /** Helper: fetch our UI subsystem for key→action mapping */
    UANSUIPlayerSubsystem* GetUISubsystem() const;
};
