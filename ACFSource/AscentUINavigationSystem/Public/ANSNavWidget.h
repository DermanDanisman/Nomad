// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ANSUITypes.h"
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "ANSNavWidget.generated.h"

struct FFocusedWidget;
class UANSNavPageWidget;

/**
 *
 */
UCLASS()
class ASCENTUINAVIGATIONSYSTEM_API UANSNavWidget : public UUserWidget {
    GENERATED_BODY()

public:
    UANSNavWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintImplementableEvent, Category = ANS)
    void HandleFocusReceived();

    UFUNCTION(BlueprintImplementableEvent, Category = ANS)
    void HandleFocusLost();

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = ANS)
    void UpdateStyle(bool bHovered);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = ANS)
    void SetStyleIndex(int32 styleIndex);

    UFUNCTION(BlueprintCallable, Category = ANS)
    void RequestFocus();

    UFUNCTION(BlueprintCallable, Category = ANS)
    void RequestFocusForWidget(/*class UANSNavPageWidget* owningPage,*/ UANSNavWidget* widgetToFocus);

    UFUNCTION(BlueprintPure, Category = ANS)
    bool GetIsHovered() const
    {
        return bIsHovered;
    }

    UFUNCTION(BlueprintPure, Category = ANS)
    UANSNavPageWidget* GetOwningPage() const
    {
        return navPage;
    }

    UFUNCTION(BlueprintImplementableEvent, BlueprintPure,Category = ANS)
    UWidget* GetFocusableSubWidget() const;

    bool GetFocusedSubWidget(FFocusedWidget& outwidget, class UANSNavPageWidget* pageOwner);

    void OnFocusReceived();

    void OnFocusLost();

    void SetPageOwner(class UANSNavPageWidget* pageOwner);


protected:
    void NativePreConstruct() override;
    void NativeConstruct() override;
    void NativeOnMouseEnter( const FGeometry& InGeometry, const FPointerEvent& InMouseEvent ) override;
    void NativeOnMouseLeave( const FPointerEvent& InMouseEvent ) override;
    void TryGetPageOwner();

private:
    TObjectPtr<class UANSNavPageWidget> navPage;

    bool bIsHovered;
};
