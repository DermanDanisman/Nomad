// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ADSDialoguePartecipantComponent.h"
#include "ADSDialogueSubsystem.h"
#include <Animation/AnimInstance.h>
#include <Components/SkeletalMeshComponent.h>
#include <Engine/GameInstance.h>
#include <GameFramework/Character.h>
#include <Kismet/GameplayStatics.h>

// Sets default values for this component's properties
UADSDialoguePartecipantComponent::UADSDialoguePartecipantComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;

    // ...
}

// Called when the game starts
void UADSDialoguePartecipantComponent::BeginPlay()
{
    Super::BeginPlay();

    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    UADSDialogueSubsystem* adsSubsystem = GameInstance->GetSubsystem<UADSDialogueSubsystem>();
    adsSubsystem->RegisterParticipant(this);

    for (const auto& dialogue : Dialogues) {
         UADSDialogue* newDialogue = DuplicateObject(dialogue, GetOuter());
        if (newDialogue) {
            instancedDialogues.Add(newDialogue);
            newDialogue->OnDialogueStarted.AddDynamic(this, &UADSDialoguePartecipantComponent::HandleDialogueStarted);
            newDialogue->OnDialogueEnded.AddDynamic(this, &UADSDialoguePartecipantComponent::HandleDialogueEnded);
        }
    }
}

void UADSDialoguePartecipantComponent::EndPlay(EEndPlayReason::Type reason)
{
    Super::EndPlay(reason);
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    UADSDialogueSubsystem* adsSubsystem = GameInstance->GetSubsystem<UADSDialogueSubsystem>();
    adsSubsystem->UnregisterParticipant(PartecipantTag);

    for (const auto& dialogue : instancedDialogues) {
        if (dialogue) {
            dialogue->OnDialogueStarted.RemoveDynamic(this, &UADSDialoguePartecipantComponent::HandleDialogueStarted);
            dialogue->OnDialogueEnded.RemoveDynamic(this, &UADSDialoguePartecipantComponent::HandleDialogueEnded);
        }
    }
}

bool UADSDialoguePartecipantComponent::TryStartDialogue(const TArray<UADSDialoguePartecipantComponent*>& participants, UADSDialogue* dialogueToStart)
{
    if (IsValid(dialogueToStart)) {
        return IsValid(dialogueToStart->StartDialogue(UGameplayStatics::GetPlayerController(this, 0), participants));
    }
    UE_LOG(LogTemp, Error, TEXT("No	ADS Dialogue Set!- UADSDialoguePartecipantComponent::TryStartDialogue"));
    return false;
}

bool UADSDialoguePartecipantComponent::TryStartDialogueFromActors(const TArray<AActor*>& participants, UADSDialogue* dialogueToStart)
{
    TArray<UADSDialoguePartecipantComponent*> participantComps;
    for (const auto actor : participants) {
        UADSDialoguePartecipantComponent* dialogueComp = actor->FindComponentByClass<UADSDialoguePartecipantComponent>();
        if (dialogueComp) {
            participantComps.Add(dialogueComp);
        } else {
            UE_LOG(LogTemp, Error, TEXT("No	ADS Dialogue Component in the participant!- UADSDialoguePartecipantComponent::TryStartDialogue"));
            return false;
        }
    }
    return TryStartDialogue(participantComps, dialogueToStart);
}

UADSDialogue* UADSDialoguePartecipantComponent::GetDialogue(FGameplayTag dialogueTag, bool& bFound) const
{
    if (!instancedDialogues.IsValidIndex(0)) {
        UE_LOG(LogTemp, Warning, TEXT("No available dialogues for this participant!- UADSDialoguePartecipantComponent::GetDialogue"));
        bFound = false;
        return nullptr;
    }

    for (const auto dial : instancedDialogues) {
        if (dial && dial->GetDialogueTag() == dialogueTag) {
            bFound = true;
            return dial;
        }
    }
    bFound = false;
    return nullptr;
}

USkeletalMeshComponent* UADSDialoguePartecipantComponent::GetOwnerMesh()
{
    if (skeletalMesh) {
        return skeletalMesh;
    }

    const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
    if (CharacterOwner) {
        skeletalMesh = CharacterOwner->GetMesh();
    }

    skeletalMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
    return skeletalMesh;
}

void UADSDialoguePartecipantComponent::PlayAnimationOnCharacterOwner(UAnimMontage* animationTag)
{
    if (GetOwnerMesh()) {

        UAnimInstance* animInstance = skeletalMesh->GetAnimInstance();

        if (animInstance) {
            animInstance->Montage_Play(animationTag);
        }
    }
}

void UADSDialoguePartecipantComponent::OnDialogueStartedEvent_Implementation()
{
}

void UADSDialoguePartecipantComponent::OnDialogueEndedEvent_Implementation()
{
}

void UADSDialoguePartecipantComponent::HandleDialogueStarted()
{
    OnDialogueStartedEvent();
    OnDialogueStarted.Broadcast();
}

void UADSDialoguePartecipantComponent::HandleDialogueEnded()
{
    OnDialogueEndedEvent();

    OnDialogueEnded.Broadcast();
}
