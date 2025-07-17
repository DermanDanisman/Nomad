// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ANSUITypes.h"
#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"
#include "Layout/Geometry.h"

#include "ANSNavPageWidget.generated.h"

class UANSNavWidget;
class UANSNavbarWidget;

/**
 *
 */

UCLASS()
class ASCENTUINAVIGATIONSYSTEM_API UANSNavPageWidget : public UCommonActivatableWidget {
    GENERATED_BODY()

public:
    /*	UANSNavPageWidget();*/
    /*Sets the initial focus for gamepad navigation to the provided widget*/
    UFUNCTION(BlueprintCallable, Category = ANS)
    void SetStartFocus(UANSNavWidget* navWidget = nullptr);

    /*Resets Focus to default focusable widget, obtained from GetDesiredFocusTarget*/
    UFUNCTION(BlueprintCallable, Category = ANS)
    void ResetDefaultFocus();

    UFUNCTION(BlueprintCallable, Category = ANS)
    void SetFocusToWidget(UANSNavWidget* navWidget = nullptr);

    UFUNCTION(BlueprintPure, Category = ANS)
    UANSNavWidget* GetFocusedWidget() const
    {
        return focusedWidget.parentNavWidget;
    }

    // ENABLE AND DISABLE GAMEPAD NAVIGATION FROM THIS PAGE//
    UFUNCTION(BlueprintCallable, Category = ANS)
    void SetNavigationEnabled(bool bNavEnabled);

    UFUNCTION(BlueprintCallable, Category = ANS)
    void DisableNavigation();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = ANS)
    bool GetNavigationEnabled() const
    {
        return bIsNavEnabled;
    }

    UFUNCTION(BlueprintPure, Category = ANS)
    UANSNavbarWidget* GetCurrentNavbar();

    // WIDGET NAVIGATION FUNCTIONS//

    /*Remove current widget from screen and spawns the provided one*/
    UFUNCTION(BlueprintCallable, Category = ANS)
    void GoToWidget(TSubclassOf<UUserWidget> nextPage);
    /*Remove current widget from screen and spawns again the last widget that have been removed*/
    UFUNCTION(BlueprintCallable, Category = ANS)
    void GoToPreviousWidget();

    UFUNCTION(BlueprintPure, Category = ANS)
    bool GetAutoSwitchFromMouseAndGamepad() const { return bAutoSwitchFromMouseAndGamepad; }

    UFUNCTION(BlueprintCallable, Category = ANS)
    void SetAutoSwitchFromMouseAndGamepad(bool val) { bAutoSwitchFromMouseAndGamepad = val; }

protected:
    UFUNCTION(BlueprintNativeEvent, Category = ANS)
    void OnFocusedWidgetChanged(const FFocusedWidget& newFocusedWidget);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UCommonInputSubsystem* GetInputSubsystem() const;
    void GatherNavbar();

    virtual void NativePreConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent, const FNavigationReply& InDefaultReply) override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    void CheckFocusedWidget(const FGeometry& MyGeometry);

    virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;

    UPROPERTY(BlueprintReadOnly, Category = ANS)
    UANSNavbarWidget* navBar;

    UPROPERTY(EditAnywhere, Category = ANS)
    bool bAutoSwitchFromMouseAndGamepad = true;

private:
    void Internal_SetFocusToWidget(const FFocusedWidget& widget, const FGeometry& MyGeometry);
    bool Internal_SetFocusToNavWidget(UANSNavWidget* widget, const FGeometry& MyGeometry);

    void Internal_SetStartFocus();
    void RemoveFocusToCurrentWidget();

    TObjectPtr<class UANSUIPlayerSubsystem> UISubsystem;

    // class UWidget* currentlyFocusedWidget;
    FFocusedWidget startFocusedWidget;
    bool bPendingFocusRequest = false;
    bool bIsNavEnabled;

    FFocusedWidget focusedWidget;

    UFUNCTION()
    void HandleInputChanged(ECommonInputType bNewInputType);
};
