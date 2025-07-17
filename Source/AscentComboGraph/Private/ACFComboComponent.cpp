// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ACFComboComponent.h"
#include "Graph/ACFComboGraph.h"
#include <EnhancedInputComponent.h>
#include <EnhancedPlayerInput.h>
#include <InputAction.h>
#include <Net/UnrealNetwork.h>
#include <UObject/CoreNet.h>
#include "Components/ACFActionsManagerComponent.h"

// Sets default values for this component's properties
UACFComboComponent::UACFComboComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;
    bIsInputBufferEnabled = false;
    bIsPerformingCombo = false;
    // ...
}

void UACFComboComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UACFComboComponent, bIsInputBufferEnabled);
    DOREPLIFETIME(UACFComboComponent, bIsPerformingCombo);
}

void UACFComboComponent::StartCombo_Implementation(UACFComboGraph* comboToStart, const FGameplayTag& triggeringAction)
{
    if (IsExecutingAnyCombo()) {
        StopCurrentCombo();
    }
    if ( comboToStart) {
        FActionState action;
        if (actionsComp->GetActionByTag(triggeringAction, action)) {
            currentCombo = comboToStart; /* DuplicateObject<UACFComboGraph>(comboToStart, GetOuter());   */           
            currentCombo->StartCombo(triggeringAction);
            bIsPerformingCombo = currentCombo->IsActive();
        }
    }
}

void UACFComboComponent::StopCombo_Implementation(UACFComboGraph* comboToStart)
{
    if (currentCombo == comboToStart) {
        StopCurrentCombo();
    }
}

void UACFComboComponent::SetInputBufferOpened_Implementation(bool bEnabled)
{
    bIsInputBufferEnabled = bEnabled;
}

bool UACFComboComponent::IsExecutingCombo(UACFComboGraph* combo) const
{
    return IsExecutingAnyCombo() && currentCombo == combo;
}

bool UACFComboComponent::IsExecutingAnyCombo() const
{
    return bIsPerformingCombo;
}

// Called when the game starts
void UACFComboComponent::BeginPlay()
{
    Super::BeginPlay();

    inputComponent = GetOwner()->FindComponentByClass<UEnhancedInputComponent>();
    if (inputComponent) {
        for (UInputAction* input : ComboInputs) {
            inputComponent->BindAction(input, ETriggerEvent::Started, this,
                &UACFComboComponent::HandleComboInputReceived, input);
        }
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Missing Enhanced Input Comp!  - UACFComboComponent"));
    }
    actionsComp = GetOwner()->FindComponentByClass<UACFActionsManagerComponent>();
    if (!actionsComp) {
        UE_LOG(LogTemp, Error, TEXT("Missing ACTIONS Comp!  - UACFComboComponent"));
    }
}

void UACFComboComponent::HandleComboInputReceived(UInputAction* input)
{
    SendInputReceived(input);
}

void UACFComboComponent::SendInputReceived_Implementation(UInputAction* input)
{
    if (bIsPerformingCombo && bIsInputBufferEnabled && currentCombo) {
        currentCombo->InputReceived(input);
    }
}

void UACFComboComponent::StopCurrentCombo()
{
    if (currentCombo) {
        currentCombo->StopCombo();
        //check memory leak
        currentCombo = nullptr;
        bIsPerformingCombo = false;
    }
}
