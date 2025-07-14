// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ACFBaseCommand.generated.h"

class AACFAIController;


/**
 *
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FonCommandFinished, bool, success);

UCLASS(Blueprintable, BlueprintType, abstract, EditInlineNew, HideCategories = ("DoNotShow"), CollapseCategories, AutoExpandCategories = ("Default"))
class AIFRAMEWORK_API UACFBaseCommand : public UObject {
    GENERATED_BODY()

public:
    friend class UACFCommandsManagerComponent;

    //
    // 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ACF)
    // 	float EstimatedDurationWait = 1.f;

    UFUNCTION(BlueprintCallable, Category = ACF)
    void FinishExecution(bool bSuccess);

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FonCommandFinished OnCommandFinished;

protected:
    void InternalExecute(class AACFAIController* controller);

    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void ExecuteCommand(class AACFAIController* controller);
    virtual void ExecuteCommand_Implementation(class AACFAIController* controller);

    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void EndCommand(class AACFAIController* controller);
    virtual void EndCommand_Implementation(class AACFAIController* controller);

    
    UWorld* GetWorld() const override;

    UFUNCTION(BlueprintPure, Category = ACF)
    AACFAIController* GetAIController() const {
        return aiController;
    }

private:
    TObjectPtr < AACFAIController> aiController;

};
