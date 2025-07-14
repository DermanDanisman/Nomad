// // Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ADSDialogueMasterComponent.h"
#include <GameFramework/PlayerController.h>
#include <GameFramework/Pawn.h>

// Sets default values for this component's properties
UADSDialogueMasterComponent::UADSDialogueMasterComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    // ...
}

void UADSDialogueMasterComponent::AddDialogueVariable(FString key, FString value)
{
    DialogueVariables.Add(key, value);
}

void UADSDialogueMasterComponent::RemoveDialogueVariable(FString key)
{
    DialogueVariables.Remove(key);
}

bool UADSDialogueMasterComponent::GetDialogueVariable(FString key, FString& outValue) const
{
    if (DialogueVariables.Contains(key)) {
        outValue = *DialogueVariables.Find(key);
        return true;
    }
    return false;
}

FText UADSDialogueMasterComponent::ReplaceVariablesInText(const FText& inText)
{
    FString outString = inText.ToString();

    // Iterate over the map
    for (const TPair<FString, FString>& Pair : DialogueVariables) {
        outString = outString.Replace(*Pair.Key, *Pair.Value, ESearchCase::IgnoreCase);
    }

    return FText::FromString(outString);
}

void UADSDialogueMasterComponent::MoveControlledPlayerToPosition_Implementation(const FTransform& finalPoint)
{
    APlayerController* pc = Cast<APlayerController>(GetOwner());
    
    if (pc && pc->GetPawn()) {
        pc->GetPawn()->SetActorLocation(finalPoint.GetLocation());
        pc->GetPawn()->SetActorRelativeRotation(finalPoint.GetRotation());
    }
   
}

// Called when the game starts
void UADSDialogueMasterComponent::BeginPlay()
{
    Super::BeginPlay();

    // ...
}
