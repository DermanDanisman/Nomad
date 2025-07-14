// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AGSGraph.h"
#include "AGSGraphRuntimePCH.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "AGSGraph"

UAGSGraph::UAGSGraph()
{
    NodeType = UAGSGraphNode::StaticClass();
    EdgeType = UAGSGraphEdge::StaticClass();

    bEdgeEnabled = true;

#if WITH_EDITORONLY_DATA
    EdGraph = nullptr;

    bCanRenameNode = true;
#endif
    ActivedNodes.Empty();
}

UAGSGraph::~UAGSGraph() { }

void UAGSGraph::Print(bool ToConsole /*= true*/, bool ToScreen /*= true*/)
{
    int Level = 0;
    TArray<UAGSGraphNode*> CurrLevelNodes = RootNodes;
    TArray<UAGSGraphNode*> NextLevelNodes;

    while (CurrLevelNodes.Num() != 0) {
        for (int i = 0; i < CurrLevelNodes.Num(); ++i) {
            UAGSGraphNode* Node = CurrLevelNodes[i];
            check(Node != nullptr);

            FString Message = FString::Printf(
                TEXT("%s, Level %d"), *Node->GetDescription().ToString(), Level);

            if (ToConsole) {
                LOG_INFO(TEXT("%s"), *Message);
            }

            if (ToScreen && GEngine != nullptr) {
                GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Blue, Message);
            }

            for (int j = 0; j < Node->ChildrenNodes.Num(); ++j) {
                NextLevelNodes.Add(Node->ChildrenNodes[j]);
            }
        }

        CurrLevelNodes = NextLevelNodes;
        NextLevelNodes.Reset();
        ++Level;
    }
}

bool UAGSGraph::ActivateNode(UAGSGraphNode* node)
{
    if (node && AllNodes.Contains(node)) {
        ActivedNodes.AddUnique(node);
        node->ActivateNode();
        return true;
    }
    return false;
}

bool UAGSGraph::DeactivateNode(UAGSGraphNode* node)
{
    if (node && AllNodes.Contains(node)) {
        node->DeactivateNode();
        ActivedNodes.Remove(node);

        return true;
    }
    return false;
}

void UAGSGraph::DeactivateAllNodes()
{
    TArray<UAGSGraphNode*> copiedVect = GetActiveNodes();
    for (auto node : copiedVect) {
        DeactivateNode(node);
    }
}

int UAGSGraph::GetLevelNum() const
{

    int Level = 0;
    TArray<UAGSGraphNode*> CurrLevelNodes = RootNodes;
    TArray<UAGSGraphNode*> NextLevelNodes;

    while (CurrLevelNodes.Num() != 0) {
        for (int i = 0; i < CurrLevelNodes.Num(); ++i) {
            UAGSGraphNode* Node = CurrLevelNodes[i];
            check(Node != nullptr);

            for (int j = 0; j < Node->ChildrenNodes.Num(); ++j) {
                NextLevelNodes.Add(Node->ChildrenNodes[j]);
            }
        }

        CurrLevelNodes = NextLevelNodes;
        NextLevelNodes.Reset();
        ++Level;
    }

    return Level;
}

void UAGSGraph::GetNodesByLevel(int Level, TArray<UAGSGraphNode*>& Nodes)
{
    int CurrLEvel = 0;
    TArray<UAGSGraphNode*> NextLevelNodes;

    Nodes = RootNodes;

    while (Nodes.Num() != 0) {
        if (CurrLEvel == Level)
            break;

        for (int i = 0; i < Nodes.Num(); ++i) {
            UAGSGraphNode* Node = Nodes[i];
            check(Node != nullptr);

            for (int j = 0; j < Node->ChildrenNodes.Num(); ++j) {
                NextLevelNodes.Add(Node->ChildrenNodes[j]);
            }
        }

        Nodes = NextLevelNodes;
        NextLevelNodes.Reset();
        ++CurrLEvel;
    }
}

void UAGSGraph::ClearGraph()
{
    for (int i = 0; i < AllNodes.Num(); ++i) {
        UAGSGraphNode* Node = AllNodes[i];

        Node->ParentNodes.Empty();
        Node->ChildrenNodes.Empty();
        Node->Edges.Empty();
    }

    AllNodes.Empty();
    RootNodes.Empty();
}

#undef LOCTEXT_NAMESPACE
