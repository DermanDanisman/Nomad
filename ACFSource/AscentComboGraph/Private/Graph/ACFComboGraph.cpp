// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Graph/ACFComboGraph.h"
#include "Actions/ACFBaseAction.h"
#include "Engine/Engine.h"
#include "Graph/ACFComboNode.h"
#include "Graph/ACFStartComboNode.h"
#include "Graph/ACFTransition.h"

bool UACFComboGraph::ActivateNode(class UAGSGraphNode* node)
{
    return Super::ActivateNode(node);
}

UACFComboGraph::UACFComboGraph()
{
    NodeType = UACFComboNode::StaticClass();
    EdgeType = UACFTransition::StaticClass();
    Enabled = EComboState::NotStarted;
}

void UACFComboGraph::StartCombo(const FGameplayTag& inStartAction)
{
    for (UAGSGraphNode* root : RootNodes) {
        UACFStartComboNode* startNode = Cast<UACFStartComboNode>(root);
        if (startNode && startNode->GetTriggeringAction() == inStartAction) {  
            Enabled = EComboState::Started;
            ActivateNode(startNode);
            return;
        }
    }
}

void UACFComboGraph::StopCombo()
{
    DeactivateAllNodes();
    Enabled = EComboState::NotStarted;
}

void UACFComboGraph::InputReceived(UInputAction* currentInput)
{
    storedInput = currentInput;

}

bool UACFComboGraph::PerformPendingTransition()
{
    if (PerformTransition(GetLastInput())) {
        storedInput = nullptr;
        return true;
    }
    return false;
   
}

bool UACFComboGraph::PerformTransition(UInputAction* currentInput)
{
    for (auto node : GetActiveNodes()) {
        UACFComboNode* comboNode = Cast<UACFComboNode>(node);
        if (node) {
            for (auto edge : comboNode->Edges) {
                const UACFTransition* transition = Cast<UACFTransition>(edge.Value);
                if (transition && transition->GetTransitionInput()) {
                    if (transition->GetTransitionInput() == currentInput) {
                        UACFComboNode* newNode = Cast<UACFComboNode>(edge.Key);
                        ensure(newNode);
                        DeactivateNode(node);
                        ActivateNode(newNode);
                        return true;
                    }
                }
            }
        }
    }
    return false;

}

FGameplayTag UACFComboGraph::GetTriggeringAction() const
{
    return triggeringAction;
}

UACFComboNode* UACFComboGraph::GetCurrentComboNode() const
{
    if (IsActive() && GetActiveNodes().IsValidIndex(0)) {
        // only one combo anim can be reproduced at time
        return Cast<UACFComboNode>(GetActiveNodes()[0]);
    }
    return nullptr;
}

UAnimMontage* UACFComboGraph::GetCurrentComboMontage() const
{
    UACFComboNode* currentNode = GetCurrentComboNode();
    if (currentNode) {
        return currentNode->GetMontage();
    }
    return nullptr;
}

bool UACFComboGraph::GetCurrentComboModifier(FAttributesSetModifier& outModifier) const
{
    UACFComboNode* currentNode = GetCurrentComboNode();
    if (currentNode) {
        outModifier = currentNode->GetComboNodeModifier();
        return true;
    }
    return false;
}


