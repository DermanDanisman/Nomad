// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Graph/ADSGraphNode.h"

#include "ADSDialogueNode.generated.h"

class ACineCameraActor;
class ATargetPoint;


/**
 *
 */
UCLASS()
class ASCENTDIALOGUESYSTEM_API UADSDialogueNode : public UADSGraphNode {
    GENERATED_BODY()


protected:
    UPROPERTY(EditAnywhere, Category = "ADS|Camera")
    TSoftObjectPtr<ACineCameraActor> CameraActor;


    UPROPERTY(EditAnywhere, Category = "ADS|Camera")
    float BlendTime = 0.02f;
    	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = ADS)
	TArray<class UAGSCondition*> ActivationConditions;

    virtual void ActivateNode() override;


public:
    UADSDialogueNode();

    UFUNCTION(BlueprintCallable, Category = ADS)
    TArray<class UADSDialogueResponseNode*> GetAllValidAnswers(APlayerController* inController);

  	virtual bool CanBeActivated(APlayerController* inController) override;
};
