// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Graph/ADSGraphNode.h"
#include "ADSDialogueDeveloperSettings.h"
#include "ADSDialoguePartecipantComponent.h"
#include "AGSAction.h"
#include "AGSCondition.h"
#include "AGSGraphNode.h"
#include "Graph/ADSDialogue.h"
#include "Graph/ADSDialogueResponseNode.h"
#include "Graph/ADSGraphEdge.h"
#include <Animation/AnimInstance.h>
#include <Animation/AnimMontage.h>
#include <Animation/AnimSequence.h>
#include <Components/AudioComponent.h>
#include <Components/SkeletalMeshComponent.h>
#include <GameFramework/PlayerController.h>
#include <Kismet/GameplayStatics.h>
#include <Sound/SoundCue.h>
#include "ADSDialogueFunctionLibrary.h"

void UADSGraphNode::ActivateNode()
{
    Super::ActivateNode();

    partecipant = GatherPartecipant();

    for (UAGSAction* action : ActivationActions) {
        if (action) {
            action->Execute(UGameplayStatics::GetPlayerController(this, 0), this);
        }
    }

    if (!partecipant) {
        UE_LOG(LogTemp, Error, TEXT("Invalid Partecipant!"));
        return;
    }

    USkeletalMeshComponent* skeletal = partecipant->GetOwnerMesh();

    if (skeletal) {
        partecipant->PlayAnimationOnCharacterOwner(Animation);
        partecipant->PlayAnimationOnCharacterOwner(FacialAnimation);

        audioPlayer = UGameplayStatics::SpawnSoundAttached(SoundToPlay, skeletal, partecipant->GetVoiceSpawningSocketName());
    }
}

void UADSGraphNode::DeactivateNode()
{
    if (audioPlayer->IsValidLowLevel() && audioPlayer->IsPlaying()) {
        audioPlayer->Stop();
    }
    Super::DeactivateNode();
}

class UADSDialoguePartecipantComponent* UADSGraphNode::GatherPartecipant() const
{
    const UADSDialogue* dialogue = Cast<UADSDialogue>(GetGraph());

    if (!dialogue) {
        return nullptr;
    }
    return dialogue->FindPartecipant(PartecipantTag);
}

class UADSDialoguePartecipantComponent* UADSGraphNode::GetDialogueParticipant() const
{
    return partecipant.Get() ? partecipant.Get() : GatherPartecipant();
}

bool UADSGraphNode::IsLocalPlayerPartecipant() const
{

    if (controller && GetDialogueParticipant()) {
        return GetDialogueParticipant()->GetOwner() == controller->GetPawn();
    }
    return false;
}

bool UADSGraphNode::CanBeActivated(APlayerController* inController)
{
    controller = inController;

    return true;
}

#if WITH_EDITOR

FText UADSGraphNode::GetNodeTitle() const
{
    if (PartecipantTag != FGameplayTag()) {

        return FText::FromString( UADSDialogueFunctionLibrary::ExtractLastStringFromGameplayTag(PartecipantTag));
    }
    return FText::FromString("Set Participant Tag!");
}

FText UADSGraphNode::GetParagraphTitle() const
{
    const FString final = Text.ToString().Left(30);
    return FText::FromString(final);
}

void UADSGraphNode::InitializeNode()
{
    Super::InitializeNode();
    const UADSDialogue* dialogue = Cast<UADSDialogue>(GetGraph());
    PartecipantTag = dialogue->GetDefaultParticipantTag();
}

#endif
