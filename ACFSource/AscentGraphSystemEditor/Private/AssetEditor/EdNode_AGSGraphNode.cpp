// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "EdNode_AGSGraphNode.h"
#include "EdGraph_AGSGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "AGSGraphNode.h"

#define LOCTEXT_NAMESPACE "EdNode_AGSGraph"

UEdNode_AGSGraphNode::UEdNode_AGSGraphNode()
{
    bCanRenameNode = true;
}

UEdNode_AGSGraphNode::~UEdNode_AGSGraphNode()
{
}

void UEdNode_AGSGraphNode::AllocateDefaultPins()
{
    CreatePin(EGPD_Input, "MultipleNodes", FName(), TEXT("In"));
    CreatePin(EGPD_Output, "MultipleNodes", FName(), TEXT("Out"));
}

UEdGraph_AGSGraph* UEdNode_AGSGraphNode::GetAGSGraphEdGraph()
{
    return Cast<UEdGraph_AGSGraph>(GetGraph());
}

FText UEdNode_AGSGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (AGSGraphNode == nullptr) {
        return Super::GetNodeTitle(TitleType);
    } else {
        return AGSGraphNode->GetNodeTitle();
    }
}

FText UEdNode_AGSGraphNode::GetParagraphTitle() const
{
    if (AGSGraphNode) {
        return AGSGraphNode->GetParagraphTitle();
    } else {
        return FText::FromString("Missing Paragraph");
    }
}

void UEdNode_AGSGraphNode::PrepareForCopying()
{
    AGSGraphNode->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UEdNode_AGSGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
    Super::AutowireNewNode(FromPin);

    if (FromPin != nullptr) {
        if (GetSchema()->TryCreateConnection(FromPin, GetInputPin())) {
            FromPin->GetOwningNode()->NodeConnectionListChanged();
        }
    }
}

void UEdNode_AGSGraphNode::SetAGSGraphNode(UAGSGraphNode* InNode)
{
    AGSGraphNode = InNode;
}

FLinearColor UEdNode_AGSGraphNode::GetBackgroundColor() const
{
    return AGSGraphNode == nullptr ? FLinearColor::Black : AGSGraphNode->GetBackgroundColor();
}

UEdGraphPin* UEdNode_AGSGraphNode::GetInputPin() const
{
    return Pins[0];
}

UEdGraphPin* UEdNode_AGSGraphNode::GetOutputPin() const
{
    return Pins[1];
}

void UEdNode_AGSGraphNode::PostEditUndo()
{
    UEdGraphNode::PostEditUndo();
}

#undef LOCTEXT_NAMESPACE
