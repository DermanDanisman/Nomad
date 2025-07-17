// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Actions/ACFAttackAction.h"
#include "Actions/ACFBaseAction.h"
#include "CoreMinimal.h"

#include "ACFComboAttackAction.generated.h"

class UACFComboGraph;
class UACFComboNode;
class UACFComboComponent;

/**
 *
 */
UCLASS()
class ASCENTCOMBOGRAPH_API UACFComboAttackAction : public UACFAttackAction {
    GENERATED_BODY()

public:
    UACFComboAttackAction();

protected:
    virtual void OnActionStarted_Implementation(const FString& contextString = "", AActor* InteractedActor = nullptr, FGameplayTag ItemSlotTag = FGameplayTag()) override;

    virtual void OnActionEnded_Implementation() override;
    /*The combo graph that needs to be executed*/
    UPROPERTY(EditAnywhere, Category = ACF)
    UACFComboGraph* Combo;

    /*if we need to consider the Input Buffer opened for the whole animation.
    If this is false, you'd need to add an OpenBufferNotify within the animation*/
    UPROPERTY(EditAnywhere, Category = ACF)
    bool bAutoOpenBuffer;

private:
    TObjectPtr<UACFComboNode> node;
    TObjectPtr<UACFComboComponent> comboComponent;

};
