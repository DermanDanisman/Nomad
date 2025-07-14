// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AGSAction.h"
#include "GameplayTagContainer.h"
#include "GroupQuestAction.generated.h"

/**
 * 
 */
UCLASS()
class NOMADDEV_API UGroupQuestAction : public UAGSAction
{
	GENERATED_BODY()
	
public:
    
    /** Tag of the previous quest that must be completed before activating this one */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GroupQuest")
    FGameplayTag QuestTag;

    /** Tag of the quest**/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GroupQuest")
    FGameplayTag ObjectiveTag;

    virtual void ExecuteAction_Implementation(APlayerController* PlayerController, UAGSGraphNode* NodeOwner);
};
