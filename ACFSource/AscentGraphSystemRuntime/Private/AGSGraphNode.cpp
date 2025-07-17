// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#include "AGSGraphNode.h"
#include "AGSGraph.h"

#define LOCTEXT_NAMESPACE "AGSGraphNode"

UAGSGraphNode::UAGSGraphNode()
{
#if WITH_EDITORONLY_DATA
	CompatibleGraphType = UAGSGraph::StaticClass();

//	BackgroundColor = FLinearColor::Black;
#endif
}

UAGSGraphNode::~UAGSGraphNode()
{
}

UAGSGraphEdge* UAGSGraphNode::GetEdge(UAGSGraphNode* ChildNode)
{
	return Edges.Contains(ChildNode) ? Edges.FindChecked(ChildNode) : nullptr;
}

FText UAGSGraphNode::GetDescription_Implementation() const
{
	return LOCTEXT("NodeDesc", "Default Node Name");
}

UWorld* UAGSGraphNode::GetWorld() const
{
	return Graph ? Graph->GetWorld() : nullptr;
}

class APlayerController* UAGSGraphNode::GetPlayerController() const
{
	return Graph ? Graph->GetPlayerController() : nullptr;
}

void UAGSGraphNode::ActivateNode()
{
	NodeState = ENodeState::Enabled;
}

void UAGSGraphNode::DeactivateNode()
{
	NodeState = ENodeState::Disabled;
}
#if WITH_EDITOR

FLinearColor UAGSGraphNode::GetBackgroundColor() const
{
	return BackgroundColor;
}

FText UAGSGraphNode::GetNodeTitle() const
{
	return  GetDescription();
}

FText UAGSGraphNode::GetParagraphTitle() const
{
	return  GetDescription();
}

void UAGSGraphNode::InitializeNode()
{
}

// void UAGSGraphNode::SetNodeTitle(const FText& NewTitle)
// {
// 	Text = NewTitle;
// }

bool UAGSGraphNode::CanCreateConnection(UAGSGraphNode* Other, FText& ErrorMessage)
{
	return true;
}

#endif

bool UAGSGraphNode::IsLeafNode() const
{
	return ChildrenNodes.Num() == 0;
}

UAGSGraph* UAGSGraphNode::GetGraph() const
{
	return Graph;
}

#undef LOCTEXT_NAMESPACE
