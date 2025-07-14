// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "AGSGraphNode.generated.h"

UENUM()
enum class ENodeState : uint8
{
	/** Node is enabled. */
	Enabled,
	/** Node is disabled. */
	Disabled,
};

class UAGSGraph;
class UAGSGraphEdge;

UCLASS(Blueprintable)
class AGSGRAPHRUNTIME_API UAGSGraphNode : public UObject
{
	GENERATED_BODY()

	friend class UAGSGraph;

protected:
	virtual void ActivateNode();

	virtual void DeactivateNode();

	UPROPERTY(BlueprintReadOnly, Category = AGS)
	ENodeState NodeState = ENodeState::Disabled;

public:

	UAGSGraphNode();
	virtual ~UAGSGraphNode();

	UPROPERTY()
	UAGSGraph* Graph;

	UPROPERTY(BlueprintReadOnly, Category = AGS)
	TArray<UAGSGraphNode*> ParentNodes;

	UPROPERTY(BlueprintReadOnly, Category = AGS)
	TArray<UAGSGraphNode*> ChildrenNodes;

	UPROPERTY(BlueprintReadOnly, Category = AGS)
	TMap<UAGSGraphNode*, UAGSGraphEdge*> Edges;

	UFUNCTION(BlueprintCallable, Category = AGS)
	UAGSGraphEdge* GetEdge(UAGSGraphNode* ChildNode);

	UFUNCTION(BlueprintCallable, Category = AGS)
	bool IsLeafNode() const;

	UFUNCTION(BlueprintCallable, Category = AGS)
	UAGSGraph* GetGraph() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AGS)
	FText GetDescription() const;
	virtual FText GetDescription_Implementation() const;


	UWorld* GetWorld() const override; 

	class APlayerController* GetPlayerController() const;
	//////////////////////////////////////////////////////////////////////////
#if WITH_EDITORONLY_DATA
	//UPROPERTY(VisibleDefaultsOnly, Category = "AGSGraphNode_Editor")
	TSubclassOf<UAGSGraph> CompatibleGraphType;

	//UPROPERTY(EditDefaultsOnly, Category = "AGSGraphNode_Editor")
	FLinearColor BackgroundColor;

	//UPROPERTY(EditDefaultsOnly, Category = "AGSGraphNode_Editor")
	FText ContextMenuName;
#endif

#if WITH_EDITOR
	virtual FLinearColor GetBackgroundColor() const;

	virtual bool CanCreateConnection(UAGSGraphNode* Other, FText& ErrorMessage);

	virtual FText GetNodeTitle() const;
	virtual FText GetParagraphTitle() const;

	virtual void InitializeNode();
/*	void SetNodeTitle(const FText& NewTitle);*/
#endif
};
