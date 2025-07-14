// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL
// 2021. All Rights Reserved.

#include "Components/ACFActionsManagerComponent.h"
#include "ACFActionTypes.h"
#include "ACFActionsFunctionLibrary.h"
#include "ARSStatisticsComponent.h"
#include "ARSTypes.h"
#include "Actions/ACFBaseAction.h"
#include "Actions/ACFSustainedAction.h"
#include "Components/SkeletalMeshComponent.h"
#include "MotionWarpingComponent.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "RootMotionModifier.h"
#include "RootMotionModifier_SkewWarp.h"
#include <Engine/Engine.h>
#include <Engine/World.h>
#include <GameFramework/Character.h>
#include <GameFramework/CharacterMovementComponent.h>
#include <GameplayTagsManager.h>
#include <Kismet/KismetSystemLibrary.h>
#include <TimerManager.h>

/*
 * Implementation of UACFActionsManagerComponent
 * 
 * This file provides the detailed logic for initializing action sets, queuing and launching actions, handling cooldowns,
 * replicating state for multiplayer, and integrating animation montages/root motion warping.
 * 
 * Design highlights:
 * - Tag-driven and data-driven, actions can be extended/modified via DataTables and Blueprints.
 * - Prioritizes network safety and flexibility for multiplayer games.
 * - Extensively commented for maintainability and extension.
 */

// Sets default values for this component's properties (enables ticking, replication, etc.)
UACFActionsManagerComponent::UACFActionsManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    SetComponentTickEnabled(false);
    ActionsSet = UACFActionsSet::StaticClass();
    CurrentPriority = -1;
}

// Initializes the component: Instantiates action sets and caches owner references.
void UACFActionsManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    // Instantiate the main action set
    if (ActionsSet)
    {
        ActionsSetInst = NewObject<UACFActionsSet>(this, ActionsSet);
    } else
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid ActionSet Class- ActionsManager"));
    }

    // Instantiate all moveset action sets (for weapon/forms)
    MovesetsActionsInst.Empty();
    for (const auto& actionssetclass : MovesetActions)
    {
        if (actionssetclass.ActionsSet)
        {
            UACFActionsSet* moveset = NewObject<UACFActionsSet>(this, actionssetclass.ActionsSet);
            MovesetsActionsInst.Add(actionssetclass.TagName, moveset);
        } else
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid ActionSet Class- ActionsManager"));
        }
    }
    CurrentPriority = -1;
    StoredAction = FGameplayTag();
    CharacterOwner = Cast<ACharacter>(GetOwner());
    if (CharacterOwner)
    {
        animInst = CharacterOwner->GetMesh()->GetAnimInstance();
        StatisticComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
        if (!StatisticComp)
        {
            UE_LOG(LogTemp, Warning, TEXT("No Statistiscs Component - ActionsManager"));
        }
    } else
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid Character - ActionsManager"));
    }
    SetComponentTickEnabled(bCanTick);
    StoredAction = FGameplayTag();
    CurrentActionTag = FGameplayTag();
}

// Instantly stops any current action and animation, and resets priority.
void UACFActionsManagerComponent::StopActionImmeditaley_Implementation()
{
    Internal_StopCurrentAnimation();
    ClientsStopActionImmeditaley();
    ExitAction();
    CurrentPriority = -1;
}

// Helper: Stops current animation montage (if any).
void UACFActionsManagerComponent::Internal_StopCurrentAnimation()
{
    FActionState action;
    if (GetActionByTag(CurrentActionTag, action))
    {
        animInst->Montage_Stop(0.0f, action.MontageAction);
    }
}

// Setup replication for all critical state variables.
void UACFActionsManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UACFActionsManagerComponent, MontageInfo);
    DOREPLIFETIME(UACFActionsManagerComponent, CurrentActionTag);
    DOREPLIFETIME(UACFActionsManagerComponent, CurrentPriority);
    DOREPLIFETIME(UACFActionsManagerComponent, bIsPerformingAction);
    DOREPLIFETIME(UACFActionsManagerComponent, currentMovesetActionsTag);
}

// On tick: if an action is ongoing, call its tick method to allow combo/charge logic.
void UACFActionsManagerComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsPerformingAction && PerformingAction)
    {
        PerformingAction->OnTick(DeltaTime);
    }
}

// Enables or disables debug state printing.
void UACFActionsManagerComponent::SetPrintDebugInfo_Implementation(bool bNewPrintDebugInfo)
{
    bPrintDebugInfo = bNewPrintDebugInfo;
}

// Enables or disables ticking.
void UACFActionsManagerComponent::SetCanTick_Implementation(bool bNewCanTick)
{
    bCanTick = bNewCanTick;
}

// Changes out the current actions set class (for runtime action table switching).
void UACFActionsManagerComponent::SetActionsSet_Implementation(TSubclassOf<UACFActionsSet> NewActionsSet)
{
    ActionsSet = NewActionsSet;
}

// Triggers an action by tag name, resolving to a GameplayTag.
void UACFActionsManagerComponent::TriggerActionByName(
    FName ActionTagName,
    EActionPriority Priority /*= EActionPriority::ELow*/, bool bCanBeStored /*= false*/, const FString& contextString)
{
    const FGameplayTag tag = UGameplayTagsManager::Get().RequestGameplayTag(ActionTagName);

    if (tag.IsValid())
    {
        TriggerAction(tag, Priority, bCanBeStored, contextString);
    }
}

// Sets the entire moveset action array.
void UACFActionsManagerComponent::SetMovesetActionArray_Implementation(const TArray<FActionsSet>& NewMovesetActions)
{
    MovesetActions = NewMovesetActions;
}

// Sets the active moveset tag.
void UACFActionsManagerComponent::SetMovesetActions_Implementation(const FGameplayTag& movesetActionsTag)
{
    currentMovesetActionsTag = movesetActionsTag;
}

// Core function to trigger an action, handling priorities, queueing, and state checks.
void UACFActionsManagerComponent::TriggerAction_Implementation(FGameplayTag ActionState,
    EActionPriority Priority /*= EActionPriority::ELow*/, bool bCanBeStored /*= false*/, const FString& contextString, AActor* InteractedActor, FGameplayTag ItemSlotTag)
{
    if (!CharacterOwner)
    {
        return;
    }

    OnActionTriggered.Broadcast(ActionState, Priority);

    FActionState action;
    if (GetActionByTag(ActionState, action) && action.Action && CanExecuteAction(ActionState, ItemSlotTag))
    {
        if (((static_cast<int32>(Priority) > CurrentPriority)) || Priority == EActionPriority::EHighest)
        {
            LaunchAction(ActionState, Priority, contextString, InteractedActor, ItemSlotTag);
        } else if (CurrentActionTag != FGameplayTag() && bCanStoreAction && bCanBeStored)
        {
            StoreAction(ActionState, contextString);
        }
    } else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Invalid Action Configuration - ActionsManager"));
    }
}

// Handles playing a montage on all clients for animation sync.
void UACFActionsManagerComponent::PlayReplicatedMontage_Implementation(const FACFMontageInfo& montageInfo)
{
    MontageInfo = montageInfo;
    ClientPlayMontage(montageInfo);
}

bool UACFActionsManagerComponent::PlayReplicatedMontage_Validate(const FACFMontageInfo& montageInfo)
{
    return true;
}

void UACFActionsManagerComponent::ClientPlayMontage_Implementation(const FACFMontageInfo& montageInfo)
{
    MontageInfo = montageInfo;
    PlayCurrentMontage();
}

// Returns whether the given action is on cooldown.
bool UACFActionsManagerComponent::IsActionOnCooldown(
    FGameplayTag action) const
{
    return onCooldownActions.Contains(action);
}

// Stores an action for later (action queueing).
void UACFActionsManagerComponent::StoreAction(FGameplayTag ActionState, const FString& contextString)
{
    StoredAction = ActionState;
    StoredString = contextString;
}

// Launches an action (interrupts current, handles priorities, starts animation, VFX, etc.)
void UACFActionsManagerComponent::LaunchAction(const FGameplayTag& ActionState,
    const EActionPriority priority, const FString& contextString, AActor* InteractedActor, const FGameplayTag& ItemSlotTag)
{
    FActionState action;

    if (GetActionByTag(ActionState, action) && action.Action)
    {
        if (PerformingAction)
        {
            action.Action->OnActionTransition(PerformingAction);
            TerminateCurrentAction();
        }
        PerformingAction = action.Action;
        CurrentActionTag = ActionState;
        bIsPerformingAction = true;
        PerformingAction->SetTerminated(false);
        CurrentPriority = static_cast<int32>(priority);
        PerformingAction->Internal_OnActivated(this, action.MontageAction, contextString, InteractedActor, ItemSlotTag);
        ClientsReceiveActionStarted(ActionState, contextString);

        if (PerformingAction && PerformingAction->ActionConfig.bPlayEffectOnActionStart)
        {
            PerformingAction->PlayEffects();
        }
    }
}

// Sets the current action tag.
void UACFActionsManagerComponent::SetCurrentAction(
    const FGameplayTag& ActionState)
{
    CurrentActionTag = ActionState;
}

// Cleans up and terminates the current action (broadcasts events, resets state).
void UACFActionsManagerComponent::TerminateCurrentAction()
{
    if (bIsPerformingAction && PerformingAction && !PerformingAction->GetTerminated())
    {
        PerformingAction->Internal_OnDeactivated();
        PerformingAction->SetTerminated(true);
        PerformingAction = nullptr;
        ClientsReceiveActionEnded(CurrentActionTag);
        CurrentActionTag = FGameplayTag();
        CurrentPriority = -1;
    }
    bIsPerformingAction = false;
}

// Notifies clients of action end, plays VFX/SFX, broadcasts events.
void UACFActionsManagerComponent::ClientsReceiveActionEnded_Implementation(
    const FGameplayTag& ActionState)
{
    PrintStateDebugInfo(false);
    FActionState action;
    if (GetActionByTag(ActionState, action) && action.Action)
    {
        action.Action->ClientsOnActionEnded();
    }
    OnActionFinished.Broadcast(ActionState);
}

// Notifies clients to instantly stop current animation/action.
void UACFActionsManagerComponent::ClientsStopActionImmeditaley_Implementation()
{
    Internal_StopCurrentAnimation();
}

// Notifies clients of action start, plays VFX/SFX, broadcasts events.
void UACFActionsManagerComponent::ClientsReceiveActionStarted_Implementation(
    const FGameplayTag& ActionState, const FString& contextString)
{
    SetCurrentAction(ActionState);
    OnActionStarted.Broadcast(ActionState);
    PrintStateDebugInfo(true);

    FActionState action;
    if (GetActionByTag(ActionState, action) && action.Action)
    {
        PerformingAction = action.Action;
        if (action.Action->GetActionConfig().bAutoStartCooldown)
        {
            StartCooldown(ActionState, PerformingAction);
        }
        action.Action->CharacterOwner = CharacterOwner;
        action.Action->ClientsOnActionStarted(contextString);
    }
}

// Returns whether the given action meets requirements, is not on cooldown, etc.
bool UACFActionsManagerComponent::CanExecuteAction(FGameplayTag ActionState, FGameplayTag ItemSlotTag) const
{
    FActionState action;
    if (GetActionByTag(ActionState, action) && action.Action && StatisticComp)
    {
        UCharacterMovementComponent* moveComp = CharacterOwner->GetCharacterMovement();
        if (moveComp && !action.Action->ActionConfig.PerformableInMovementModes.Contains(moveComp->MovementMode))
        {
            UE_LOG(LogTemp, Warning, TEXT("Actions Can't be exectuted while in air!"));
            return false;
        }

        if (StatisticComp->CheckCosts(action.Action->ActionConfig.ActionCost) &&
            StatisticComp->CheckPrimaryAttributesRequirements(action.Action->ActionConfig.Requirements) &&
            !IsActionOnCooldown(ActionState) && !bIsLocked && action.Action->CanExecuteAction(CharacterOwner, ItemSlotTag) &&
            StatisticComp->GetCurrentLevel() >= action.Action->ActionConfig.RequiredLevel)
        {
            return true;
        }
        UE_LOG(LogTemp, Warning, TEXT("Actions Costs OR Actions Attribute Requirements are not verified"));
    } else
    {
        UE_LOG(LogTemp, Warning, TEXT("Actions Conditions are not verified"));
    }
    return false;
}

// Internal exit logic: cleans up current action, launches queued action if present.
void UACFActionsManagerComponent::InternalExitAction()
{
    if (bIsPerformingAction && PerformingAction)
    {
        TerminateCurrentAction();
        if (StoredAction != FGameplayTag())
        {
            TriggerAction(StoredAction, EActionPriority::EMedium, false, StoredString);
            StoreAction(FGameplayTag());
        } else
        {
            SetCurrentAction(FGameplayTag());
            ClientsReceiveActionStarted(FGameplayTag(), "");
            PerformingAction = nullptr;
        }
    }
}

// Exits the current action from Blueprint/C++.
void UACFActionsManagerComponent::ExitAction()
{
    InternalExitAction();
}

// For sustained actions: triggers release logic if currently executing.
void UACFActionsManagerComponent::ReleaseSustainedAction_Implementation(FGameplayTag actionTag)
{
    if (PerformingAction && PerformingAction->GetActionTag() == actionTag)
    {
        UACFSustainedAction* sustAction = Cast<UACFSustainedAction>(PerformingAction);
        if (sustAction)
        {
            sustAction->ReleaseAction();
        }
    }
}

// For sustained actions: plays a montage section without terminating.
void UACFActionsManagerComponent::PlayMontageSectionFromAction_Implementation(FGameplayTag actionTag, FName montageSection)
{
    if (PerformingAction && PerformingAction->GetActionTag() == actionTag)
    {
        UACFSustainedAction* sustAction = Cast<UACFSustainedAction>(PerformingAction);
        if (sustAction)
        {
            sustAction->PlayActionSection(montageSection);
        }
    }
}

// Ends current action, launches queued action if present.
void UACFActionsManagerComponent::FreeAction()
{
    CurrentPriority = -1;

    if (StoredAction != FGameplayTag())
    {
        TriggerAction(StoredAction, EActionPriority::ELow);
        StoreAction(FGameplayTag());
    } else
    {
        ExitAction();
    }
}

// Gets a moveset-specific action by tag.
bool UACFActionsManagerComponent::GetMovesetActionByTag(const FGameplayTag& action, const FGameplayTag& Moveset, FActionState& outAction) const
{
    if (MovesetsActionsInst.Contains(Moveset))
    {
        UACFActionsSet* actionSet = MovesetsActionsInst.FindChecked(Moveset);
        if (actionSet)
        {
            return actionSet->GetActionByTag(action, outAction);
        }
    }
    return false;
}

// Gets a common action by tag.
bool UACFActionsManagerComponent::GetCommonActionByTag(const FGameplayTag& action, FActionState& outAction) const
{
    if (ActionsSetInst)
    {
        return ActionsSetInst->GetActionByTag(action, outAction);
    }
    return false;
}

// Adds or modifies an action in the current action set.
void UACFActionsManagerComponent::AddOrModifyAction(const FActionState& action)
{
    if (ActionsSetInst)
    {
        return ActionsSetInst->AddOrModifyAction(action);
    }
}

// Sets the current action execution priority.
void UACFActionsManagerComponent::SetCurrentPriority(EActionPriority newPriority)
{
    CurrentPriority = static_cast<int32>(newPriority);
}

// Gets the tag of the current action.
FGameplayTag UACFActionsManagerComponent::GetCurrentActionTag() const
{
    return CurrentActionTag;
}

// Gets the action state by tag (searches moveset and common actions).
bool UACFActionsManagerComponent::GetActionByTag(const FGameplayTag& Action, FActionState& outAction) const
{
    if (ActionsSetInst)
    {
        if (GetMovesetActionByTag(Action, currentMovesetActionsTag, outAction))
        {
            return true;
        }
        if (GetCommonActionByTag(Action, outAction))
        {
            return true;
        }
    } else
    {
        return false;
    }
    return false;
}

// Plays the effects for the current action (VFX/SFX).
void UACFActionsManagerComponent::PlayCurrentActionFX()
{
    if (PerformingAction)
    {
        PerformingAction->PlayEffects();
    }
}

// Returns true if currently in a substate (e.g., parry, charge).
bool UACFActionsManagerComponent::IsInActionSubstate() const
{
    return PerformingAction && PerformingAction->bIsInSubState;
}

// Called when a notable point in the action is reached during animation.
void UACFActionsManagerComponent::AnimationsReachedNotablePoint()
{
    if (bIsPerformingAction && PerformingAction && PerformingAction->bIsExecutingAction && CharacterOwner)
    {
        if (CharacterOwner->HasAuthority())
        {
            PerformingAction->OnNotablePointReached();
            PerformingAction->ClientsOnNotablePointReached();
        } else
        {
            PerformingAction->ClientsOnNotablePointReached();
        }
    }
}

// Starts a substate for the current action.
void UACFActionsManagerComponent::StartSubState()
{
    if (bIsPerformingAction && PerformingAction && PerformingAction->bIsExecutingAction && CharacterOwner)
    {
        PerformingAction->bIsInSubState = true;
        if (CharacterOwner->HasAuthority())
        {
            PerformingAction->OnSubActionStateEntered();
            PerformingAction->ClientsOnSubActionStateEntered();
        } else
        {
            PerformingAction->ClientsOnSubActionStateEntered();
        }
    }
}

// Ends a substate for the current action.
void UACFActionsManagerComponent::EndSubState()
{
    if (PerformingAction && PerformingAction->bIsExecutingAction && CharacterOwner)
    {
        PerformingAction->bIsInSubState = false;
        if (CharacterOwner->HasAuthority())
        {
            PerformingAction->OnSubActionStateExited();
            PerformingAction->ClientsOnSubActionStateExited();
        } else
        {
            PerformingAction->ClientsOnSubActionStateExited();
        }
    }
}

// Prints debug info for action transitions (if enabled).
void UACFActionsManagerComponent::PrintStateDebugInfo(bool bIsEntring)
{
    if (bPrintDebugInfo && GEngine && CharacterOwner)
    {
        FString ActionName;
        CurrentActionTag.GetTagName().ToString(ActionName);
        FString MessageToPrint;
        if (bIsEntring)
        {
            MessageToPrint = CharacterOwner->GetName() + FString(" Entered State:") + ActionName;
        } else
        {
            MessageToPrint = CharacterOwner->GetName() + FString(" Exited State:") + ActionName;
        }

        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, MessageToPrint,
            false);
    }
}

// Core logic for playing the current montage (handles root motion, warping, etc.)
void UACFActionsManagerComponent::PlayCurrentMontage()
{
    if (MontageInfo.MontageAction && CharacterOwner)
    {
        CharacterOwner->SetAnimRootMotionTranslationScale(1.f);
        UMotionWarpingComponent* motionComp = CharacterOwner->FindComponentByClass<UMotionWarpingComponent>();
        if (motionComp)
        {
            motionComp->RemoveWarpTarget(MontageInfo.WarpInfo.WarpConfig.SyncPoint);
        }
        switch (MontageInfo.ReproductionType)
        {
        case EMontageReproductionType::ERootMotionScaled:
            CharacterOwner->SetAnimRootMotionTranslationScale(MontageInfo.RootMotionScale);
            break;
        case EMontageReproductionType::ERootMotion:
            break;
        case EMontageReproductionType::EMotionWarped:
            PrepareWarp();
            break;
        }

        CharacterOwner->PlayAnimMontage(MontageInfo.MontageAction, MontageInfo.ReproductionSpeed, MontageInfo.StartSectionName);
    }
}

// Prepares motion warping for root motion montages.
void UACFActionsManagerComponent::PrepareWarp()
{
    UMotionWarpingComponent* motionComp = CharacterOwner->FindComponentByClass<UMotionWarpingComponent>();
    const FTransform targetTransform = FTransform(MontageInfo.WarpInfo.WarpRotation, MontageInfo.WarpInfo.WarpLocation);

    if (motionComp && MontageInfo.WarpInfo.WarpConfig.bAutoWarp)
    {
        FMotionWarpingTarget targetPoint;
        if (MontageInfo.WarpInfo.WarpConfig.TargetType == EWarpTargetType::ETargetComponent && MontageInfo.WarpInfo.TargetComponent)
        {
            targetPoint = FMotionWarpingTarget(MontageInfo.WarpInfo.WarpConfig.SyncPoint, MontageInfo.WarpInfo.TargetComponent, NAME_None, MontageInfo.WarpInfo.WarpConfig.bMagneticFollow);
        } else
        {
            targetPoint = FMotionWarpingTarget(MontageInfo.WarpInfo.WarpConfig.SyncPoint, targetTransform);
        }
        motionComp->AddOrUpdateWarpTarget(targetPoint);

        URootMotionModifier_SkewWarp::AddRootMotionModifierSkewWarp(motionComp, MontageInfo.MontageAction, MontageInfo.WarpInfo.WarpConfig.WarpStartTime,
            MontageInfo.WarpInfo.WarpConfig.WarpEndTime, MontageInfo.WarpInfo.WarpConfig.SyncPoint, EWarpPointAnimProvider::None, targetTransform, NAME_None, true, true, true,
            MontageInfo.WarpInfo.WarpConfig.RotationType, EMotionWarpRotationMethod::Slerp, MontageInfo.WarpInfo.WarpConfig.WarpRotationTime);
        if (bPrintDebugInfo)
        {
            UKismetSystemLibrary::DrawDebugSphere(this, MontageInfo.WarpInfo.WarpLocation, 100.f, 12, FLinearColor::Red, 5.f);
        }
    } else
    {
        motionComp->RemoveWarpTarget(MontageInfo.WarpInfo.WarpConfig.SyncPoint);
    }
}

// Starts a cooldown for the given action, preventing immediate retriggering.
void UACFActionsManagerComponent::StartCooldown(const FGameplayTag& action, UACFBaseAction* actionRef)
{
    if (actionRef->GetActionConfig().CoolDownTime == 0.f)
    {
        return;
    }

    FTimerDelegate TimerDel;
    FTimerHandle TimerHandle;
    TimerDel.BindUFunction(this, FName("OnCooldownFinished"), action);

    UWorld* world = GetWorld();
    if (world)
    {
        onCooldownActions.Add(action);
        world->GetTimerManager().SetTimer(
            TimerHandle, TimerDel, actionRef->GetActionConfig().CoolDownTime,
            false);
        actionRef->CooldownTimerReference = TimerHandle;
    }
}

// Handler for montage info replication.
void UACFActionsManagerComponent::OnRep_MontageInfo()
{
    // PlayCurrentMontage();
}

// Removes the action from cooldowns when timer elapses.
void UACFActionsManagerComponent::OnCooldownFinished(
    const FGameplayTag& action)
{
    if (onCooldownActions.Contains(action))
    {
        onCooldownActions.Remove(action);
    }
}