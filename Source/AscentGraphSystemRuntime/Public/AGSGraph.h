// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "AGSGraphEdge.h"
#include "AGSGraphNode.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include <GameFramework/PlayerController.h>

#include "AGSGraph.generated.h"

class UEdGraph;


UCLASS(Blueprintable)
class AGSGRAPHRUNTIME_API UAGSGraph : public UObject {
    GENERATED_BODY()

public:
    UAGSGraph();
    virtual ~UAGSGraph();

    // UPROPERTY(EditDefaultsOnly, Category = "AGSGraph")
    FString Name;

    // UPROPERTY(EditDefaultsOnly, Category = "AGSGraph")
    TSubclassOf<UAGSGraphNode> NodeType;

    // UPROPERTY(EditDefaultsOnly, Category = "AGSGraph")
    TSubclassOf<UAGSGraphEdge> EdgeType;

    UWorld* GetWorld() const override { return controller ? controller->GetWorld() : nullptr; }

    //	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGSGraph")
    bool bEdgeEnabled;

    bool bAllowCycles = true;

    //	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGSGraph")
    FGameplayTagContainer GraphTags;

protected:
    TObjectPtr<APlayerController> controller;

public:
    UPROPERTY(BlueprintReadOnly, Category = "AGSGraph")
    TArray<UAGSGraphNode*> RootNodes;

    UPROPERTY(BlueprintReadOnly, Category = "AGSGraph")
    TArray<UAGSGraphNode*> AllNodes;

    UFUNCTION(BlueprintCallable, Category = "AGSGraph")
    const TArray<UAGSGraphNode*> GetAllNodes() const
    {
        return AllNodes;
    }

    UFUNCTION(BlueprintCallable, Category = "AGSGraph")
    const TArray<UAGSGraphNode*> GetActiveNodes() const
    {
        return ActivedNodes;
    }

    UFUNCTION(BlueprintCallable, Category = "AGSGraph")
    void Print(bool ToConsole = true, bool ToScreen = true);

    UFUNCTION(BlueprintCallable, Category = "AGSGraph")
    virtual bool ActivateNode(class UAGSGraphNode* node);

    UFUNCTION(BlueprintCallable, Category = "AGSGraph")
    virtual bool DeactivateNode(class UAGSGraphNode* node);

    UFUNCTION(BlueprintCallable, Category = "AGSGraph")
    void DeactivateAllNodes();

    UFUNCTION(BlueprintCallable, Category = "AGSGraph")
    int GetLevelNum() const;

    UFUNCTION(BlueprintCallable, Category = "AGSGraph")
    void GetNodesByLevel(int Level, TArray<UAGSGraphNode*>& Nodes);

    void ClearGraph();

    UFUNCTION(BlueprintPure, Category = "AGSGraph")
    class APlayerController* GetPlayerController() const
    {
        return controller;
    }

#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TObjectPtr<UEdGraph> EdGraph;

    UPROPERTY()
    bool bCanRenameNode = false;
#endif

private:
    UPROPERTY()
    TArray<UAGSGraphNode*> ActivedNodes;
};
