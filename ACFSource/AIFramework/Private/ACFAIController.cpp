// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ACFAIController.h"
#include "ATSAITargetComponent.h"
#include "ATSTargetingComponent.h"
#include "Actors/ACFActor.h"
#include "Components/ACFAIPatrolComponent.h"
#include "Components/ACFCombatBehaviourComponent.h"
#include "Components/ACFCommandsManagerComponent.h"
#include "Components/ACFDamageHandlerComponent.h"
#include "Components/ACFGroupAIComponent.h"
#include "Components/ACFInteractionComponent.h"
#include "Components/ACFQuadrupedMovementComponent.h"
#include "Components/ACFThreatManagerComponent.h"
#include "Game/ACFFunctionLibrary.h"
#include "Game/ACFGameState.h"
#include "Game/ACFTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Groups/ACFAIGroupSpawner.h"
#include "Groups/ACFGroupAgentInterface.h"
#include "Interfaces/ACFEntityInterface.h"
#include "Kismet/KismetMathLibrary.h"
#include <AI/NavigationSystemBase.h>
#include <BehaviorTree/BehaviorTree.h>
#include <BehaviorTree/BehaviorTreeComponent.h>
#include <BehaviorTree/Blackboard/BlackboardKey.h>
#include <BehaviorTree/Blackboard/BlackboardKeyType_Bool.h>
#include <BehaviorTree/Blackboard/BlackboardKeyType_Enum.h>
#include <BehaviorTree/Blackboard/BlackboardKeyType_Float.h>
#include <BehaviorTree/Blackboard/BlackboardKeyType_Int.h>
#include <BehaviorTree/Blackboard/BlackboardKeyType_Object.h>
#include <BehaviorTree/Blackboard/BlackboardKeyType_Vector.h>
#include <BehaviorTree/BlackboardComponent.h>
#include <BehaviorTree/BlackboardData.h>
#include <Components/SplineComponent.h>
#include <Engine/Engine.h>
#include <Navigation/CrowdFollowingComponent.h>
#include <Navigation/PathFollowingComponent.h>
#include <NavigationSystem.h>
#include <Perception/AIPerceptionComponent.h>
#include <Perception/AIPerceptionTypes.h>
#include <Perception/AISenseConfig_Sight.h>
#include <Perception/AISense_Hearing.h>

AACFAIController::AACFAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComp"));
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackBoardComp"));
    CommandsManagerComp = CreateDefaultSubobject<UACFCommandsManagerComponent>(TEXT("CommandManager"));
    TargetingComponent = CreateDefaultSubobject<UATSAITargetComponent>(TEXT("TargetComp"));
    //	CombatBehaviorComponent = CreateDefaultSubobject<UACFCombatBehaviourComponent>(TEXT("CombatBehav"));
    ThreatComponent = CreateDefaultSubobject<UACFThreatManagerComponent>(TEXT("ThreatManager"));
}

void AACFAIController::OnPossess(APawn* possPawn)
{
    Super::OnPossess(possPawn);
    CharacterOwned = Cast<AACFCharacter>(possPawn);
    if (!CharacterOwned) {
        UE_LOG(LogTemp, Error, TEXT("AACFAIController possess non - AACEnemyCharacter type, %s"), *possPawn->GetName());
        return;
    }

    CharacterOwned->OnDamageReceived.AddDynamic(this, &AACFAIController::HandleCharacterHit);
    CharacterOwned->GetDamageHandlerComponent()->OnOwnerDeath.AddDynamic(this, &AACFAIController::HandleCharacterDeath);

    if (!BehaviorTree) {
        UE_LOG(LogTemp, Warning, TEXT("This AACEnemyCharacter should be assigned with a behavior Tree, %s"), *possPawn->GetName());
        return;
    }

    UBlackboardData* const bbData = BehaviorTree->BlackboardAsset;
    if (bbData == nullptr) {
        UE_LOG(LogTemp, Warning, TEXT("This behavior Tree should be assigned with a blackborad, %s"), *possPawn->GetName());
        return;
    }

    BlackboardComponent->InitializeBlackboard(*bbData);
    Blackboard = BlackboardComponent;

    // Fetch the Keys
    targetActorKey = BlackboardComponent->GetKeyID("TargetActor");
    targetPointLocationKey = BlackboardComponent->GetKeyID("TargetLocation");
    targetActorDistanceKey = BlackboardComponent->GetKeyID("TargetActorDistance");
    aiStateKey = BlackboardComponent->GetKeyID("AIState");
    targetDistanceKey = BlackboardComponent->GetKeyID("TargetLocationDistance");
    commandDurationTimeKey = BlackboardComponent->GetKeyID("CommandDuration");
    groupLeaderKey = BlackboardComponent->GetKeyID("GroupLeader");
    combatStateKey = BlackboardComponent->GetKeyID("CombatState");
    isPausedKey = BlackboardComponent->GetKeyID("Paused");
    homeDistanceKey = BlackboardComponent->GetKeyID("HomeDistance");
    groupLeadDistanceKey = BlackboardComponent->GetKeyID("GroupLeadDistance");

    homeLocation = possPawn->GetActorLocation();
    BlackboardComponent->SetValue<UBlackboardKeyType_Float>(targetDistanceKey, MAX_FLT);
    BlackboardComponent->SetValue<UBlackboardKeyType_Vector>(targetPointLocationKey, homeLocation);
    BlackboardComponent->SetValue<UBlackboardKeyType_Bool>(isPausedKey, false);

    SetCurrentAIStateBK(DefaultState);
    // Launch behavior Tree
    BehaviorTreeComponent->StartTree(*BehaviorTree);

    if (PerceptionComponent) {
        PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AACFAIController::HandlePerceptionUpdated);
    }

    if (ThreatComponent) {
        ThreatComponent->OnNewMaxThreateningActor.AddDynamic(this, &AACFAIController::HandleMaxThreatUpdated);
    }

    EnableCharacterComponents(false);
}

void AACFAIController::OnUnPossess()
{
    Super::OnUnPossess();

    if (PerceptionComponent) {
        PerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &AACFAIController::HandlePerceptionUpdated);
    }

    if (ThreatComponent) {
        ThreatComponent->OnNewMaxThreateningActor.RemoveDynamic(this, &AACFAIController::HandleMaxThreatUpdated);
    }
}

void AACFAIController::EnableCharacterComponents(bool bEnabled)
{
    UACFInteractionComponent* interComponent = CharacterOwned->FindComponentByClass<UACFInteractionComponent>();

    if (interComponent) {
        if (bEnabled) {
            interComponent->RegisterComponent();
        } else {
            interComponent->UnregisterComponent();
        }
    }
}

TObjectPtr<UACFAIPatrolComponent> AACFAIController::TryGetPatrolComp() const
{
    if (IsValid(patrolComp)) {
        return patrolComp;
    }
    if (IsValid(CharacterOwned)) {
        return CharacterOwned->FindComponentByClass<UACFAIPatrolComponent>();
    }
    return nullptr;
}

void AACFAIController::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!bIsAggressive) {
        return;
    }

    if (!Actor || Actor == GetPawn()) {
        return;
    }

    if (GetPawn()->GetDistanceTo(Actor) > LoseTargetDistance) {
        return;
    }

    if (GetHomeDistanceBK() > MaxDistanceFromHome && bBoundToHome) {
        return;
    }

    // Checking if our enemy is a group
    AACFAIGroupSpawner* spawner = Cast<AACFAIGroupSpawner>(Actor);
    if (spawner) {
        Actor = spawner->GetAgentNearestTo(CharacterOwned->GetActorLocation());
        if (!Actor || Actor == GetPawn()) {
            return;
        }
    }
    IACFEntityInterface* entity = Cast<IACFEntityInterface>(Actor);

    if (entity && IACFEntityInterface::Execute_IsEntityAlive(Actor) && UACFFunctionLibrary::AreEnemyTeams(GetWorld(), CombatTeam, IACFEntityInterface::Execute_GetEntityCombatTeam(Actor))) {
        // select if it's a newly discovered potential threat

        if (!ThreatComponent->IsThreatening(Actor) && ThreatComponent->IsActorAPotentialThreat(Actor)) {
            const float threat = ThreatComponent->GetDefaultThreatForActor(Actor);
            if (threat == 0.f) {
                return;
            }
            ThreatComponent->AddThreat(Actor, threat);
        }
    }
}

void AACFAIController::HandleMaxThreatUpdated(AActor* newMax)
{
    SetTarget(newMax);
}

void AACFAIController::BeginPlay()
{
    Super::BeginPlay();
    CombatBehaviorComponent = FindComponentByClass<UACFCombatBehaviourComponent>();
}

void AACFAIController::EndPlay(const EEndPlayReason::Type reason)
{
    Super::EndPlay(reason);
    AACFGameState* gameState = GetWorld()->GetGameState<AACFGameState>();
    if (gameState) {
        gameState->RemoveAIFromBattle(this);
    }
}

void AACFAIController::TriggerCommand(const FGameplayTag& command)
{
    if (CommandsManagerComp) {
        CommandsManagerComp->TriggerCommand(command);
    }
}

class UACFGroupAIComponent* AACFAIController::GetOwningGroup_Implementation()
{
    return GroupOwner;
}

bool AACFAIController::IsPartOfGroup_Implementation()
{
    return GroupOwner != nullptr;
}

void AACFAIController::SetPatrolPath(AACFPatrolPath* inPatrol, bool forcePathFollowing)
{
    // patrolPath = inPatrol;
    if (!IsValid(inPatrol)) {
        return;
    }
    patrolComp = TryGetPatrolComp();
    if (patrolComp) {
        patrolComp->SetPathToFollow(inPatrol);
        if (forcePathFollowing) {
            patrolComp->SetPatrolType(EPatrolType::EFollowSpline);
        }
    }
}

void AACFAIController::ResetToDefaultState()
{
    SetCurrentAIStateBK(DefaultState);
}

void AACFAIController::SetWaitDurationTimeBK(float time)
{
    if (Blackboard)
        Blackboard->SetValue<UBlackboardKeyType_Float>(commandDurationTimeKey, time);
}

float AACFAIController::GetCommandDurationTimeBK() const
{
    if (Blackboard)
        return Blackboard->GetValue<UBlackboardKeyType_Float>(commandDurationTimeKey);
    return -1.f;
}

void AACFAIController::SetLeadActorBK(AActor* target)
{
    BlackboardComponent->SetValue<UBlackboardKeyType_Object>(groupLeaderKey, target);
}

FVector AACFAIController::GetTargetPointLocationBK() const
{
    return BlackboardComponent->GetValue<UBlackboardKeyType_Vector>(targetPointLocationKey);
}

bool AACFAIController::GetIsPausedBK() const
{
    return BlackboardComponent->GetValue<UBlackboardKeyType_Bool>(isPausedKey);
}

AActor* AACFAIController::GetLeadActorBK() const
{
    if (Blackboard) {
        AActor* lead = Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(groupLeaderKey));
        return lead;
    }
    return nullptr;
}

void AACFAIController::SetTargetActorBK(AActor* target)
{
    if (Blackboard)
        Blackboard->SetValue<UBlackboardKeyType_Object>(targetActorKey, target);
}

void AACFAIController::SetTargetLocationBK(const FVector& targetLocation)
{
    if (Blackboard)
        Blackboard->SetValue<UBlackboardKeyType_Vector>(targetPointLocationKey, targetLocation);
}

void AACFAIController::SetCurrentAIStateBK(EAIState aiState)
{

    if (GetAIStateBK() == aiState) {
        return;
    }

    if (GetAIStateBK() == EAIState::EBattle && CombatBehaviorComponent && PerceptionComponent && aiState != EAIState::EBattle) {
        CombatBehaviorComponent->UninitBehavior();
    }

    if (Blackboard) {
        Blackboard->SetValue<UBlackboardKeyType_Enum>(aiStateKey, static_cast<UBlackboardKeyType_Enum::FDataType>(aiState));
    }

    if (aiState == EAIState::EFollowLeader) {
        SetFocus(GetLeadActorBK(), EAIFocusPriority::Gameplay);
    }

    if (aiState == EAIState::EReturnHome) {
        SetTargetLocationBK(homeLocation);
    }

    if (aiState == EAIState::EBattle && CombatBehaviorComponent) {
        CombatBehaviorComponent->InitBehavior(this);
    }

    UpdateLocomotionState();

    // GameState settings
    AACFGameState* gameState = GetWorld()->GetGameState<AACFGameState>();
    if (!gameState) {
        return;
    }

    if (aiState == EAIState::EBattle) {
        gameState->AddAIToBattle(this);
    } else {
        gameState->RemoveAIFromBattle(this);
    }
    OnAIStateChanged.Broadcast(aiState);
}

void AACFAIController::UpdateLocomotionState()
{
    ELocomotionState* locstate = LocomotionStateByAIState.Find(GetAIStateBK());
    if (locstate && CharacterOwned && CharacterOwned->GetACFCharacterMovementComponent()) {
        CharacterOwned->GetACFCharacterMovementComponent()->SetLocomotionState(*locstate);
    }
}

AActor* AACFAIController::GetTargetActorBK() const
{
    if (Blackboard) {
        AActor* targetActor = Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(targetActorKey));
        return targetActor;
    }
    return nullptr;
}

EAIState AACFAIController::GetAIStateBK() const
{
    return static_cast<EAIState>(Blackboard->GetValue<UBlackboardKeyType_Enum>(aiStateKey));
}

EAICombatState AACFAIController::GetCombatStateBK() const
{
    return static_cast<EAICombatState>(Blackboard->GetValue<UBlackboardKeyType_Enum>(combatStateKey));
}

void AACFAIController::SetCombatStateBK(EAICombatState aiState)
{
    if (Blackboard) {
        Blackboard->SetValue<UBlackboardKeyType_Enum>(combatStateKey, static_cast<UBlackboardKeyType_Enum::FDataType>(aiState));
    }

    UpdateCombatLocomotion();

    OnAICombatStateChanged.Broadcast(aiState);
}

void AACFAIController::UpdateCombatLocomotion()
{
    if (CombatBehaviorComponent && CharacterOwned) {
        const FAICombatStateConfig* locstate = CombatBehaviorComponent->CombatStatesConfig.FindByKey(GetCombatStateBK());
        if (locstate && CharacterOwned->GetACFCharacterMovementComponent()) {
            CharacterOwned->GetACFCharacterMovementComponent()->SetLocomotionState(locstate->LocomotionState);
        }
    }
}

float AACFAIController::GetTargetActorDistanceBK() const
{
    if (Blackboard) {
        return Blackboard->GetValue<UBlackboardKeyType_Float>(targetActorDistanceKey);
    }

    return -1.f;
}

float AACFAIController::GetTargetPointDistanceBK() const
{
    if (Blackboard) {
        return Blackboard->GetValue<UBlackboardKeyType_Float>(targetDistanceKey);
    }
    return -1.f;
}

float AACFAIController::GetLeadActorDistanceBK() const
{
    if (Blackboard) {
        return Blackboard->GetValue<UBlackboardKeyType_Float>(groupLeadDistanceKey);
    }
    return -1.f;
}

float AACFAIController::GetHomeDistanceBK() const
{
    if (Blackboard) {
        return Blackboard->GetValue<UBlackboardKeyType_Float>(homeDistanceKey);
    }
    return -1.f;
}

void AACFAIController::SetTargetPointDistanceBK(float distance)
{
    if (Blackboard) {
        Blackboard->SetValue<UBlackboardKeyType_Float>(targetDistanceKey, distance);
    }
}

void AACFAIController::SetTargetActorDistanceBK(float distance)
{
    if (Blackboard) {
        Blackboard->SetValue<UBlackboardKeyType_Float>(targetActorDistanceKey, distance);
    }
}

void AACFAIController::SetIsPausedBK(bool isPaused)
{
    if (Blackboard) {
        Blackboard->SetValue<UBlackboardKeyType_Bool>(isPausedKey, isPaused);
    }
}

void AACFAIController::SetLeadActorDistanceBK(float distance)
{
    if (Blackboard) {
        Blackboard->SetValue<UBlackboardKeyType_Float>(groupLeadDistanceKey, distance);
    }
}

void AACFAIController::SetHomeDistanceBK(float distance)
{
    if (Blackboard) {
        Blackboard->SetValue<UBlackboardKeyType_Float>(homeDistanceKey, distance);
    }
}

float AACFAIController::GetPathDistanceFromTarget()
{
    return GetPathFollowingComponent()->GetRemainingPathCost();
}

bool AACFAIController::TryGoToNextWaypoint()
{
    if (!CharacterOwned) {
        return false;
    }
    if (!patrolComp) {
        patrolComp = TryGetPatrolComp();
        if (!patrolComp) {
            return false;
        }
    }

    FVector outLoc;
    if (patrolComp->TryGetNextWaypoint(outLoc)) {
        SetTargetLocationBK(outLoc);
        SetWaitDurationTimeBK(patrolComp->GetWaitTime());
        return true;
    }

    return false;
}

void AACFAIController::TeleportNearLead()
{
    const AActor* lead = GetLeadActorBK();
    if (lead) {
        const FVector homeLoc = lead->GetActorLocation();
        const APawn* pawnLead = Cast<APawn>(lead);
        FVector direction;
        if (pawnLead) {
            direction = pawnLead->GetControlRotation().RotateVector(FVector(-1.0f, 0.f, 0.f));
        } else {
            direction = lead->GetActorRotation().RotateVector(FVector(-1.0f, 0.f, 0.f));
        }
        const FVector startPoint = UACFFunctionLibrary::GetPointAtDirectionAndDistanceFromActor(lead, direction, TeleportNearLeadRadius);

        FVector outLoc;
        if (CharacterOwned && UNavigationSystemV1::K2_GetRandomLocationInNavigableRadius(this, startPoint, outLoc, 250.f)) {
            FHitResult hitRes;
            CharacterOwned->SetActorLocation(outLoc, false, &hitRes, ETeleportType::TeleportPhysics);
        }
    }
}

bool AACFAIController::TryUpdateLeadRef()
{
    if (IsPartOfGroup() && GetOwningGroup()) {
        AActor* lead = GetOwningGroup()->GetGroupLead();
        if (lead) {
            SetLeadActorBK(lead);
            return true;
        }
    }
    return false;
}

void AACFAIController::SetHomeLocation(const FVector& newHomeLocation)
{
    homeLocation = newHomeLocation;
}

void AACFAIController::OnTargetDeathHandle()
{
    if (CharacterOwned->IsAlive()) {
        AActor* target = ThreatComponent->GetActorWithHigherThreat();
        if (target) {
            SetTarget(target);
        } else {
            UE_LOG(LogTemp, Log, TEXT("%s is requesting new target - AACFAIController::OnTargetDeathHandle"), *this->GetName());
            RequestAnotherTarget();
        }
    }
}

void AACFAIController::SetGroupOwner(class UACFGroupAIComponent* group, int32 groupIndex, bool disablePerception, bool bOverrideTeam)
{
    if (group) {
        GroupOwner = group;
        GroupIndex = groupIndex;

        UAIPerceptionComponent* perception = FindComponentByClass<UAIPerceptionComponent>();
        if (perception && disablePerception) {
            perception->UnregisterComponent();
        }
    }
}

/*void AACFAIController::SetTarget(AActor* inTarget)
{
    AActor* currentTarget = GetTargetActorBK();
    IACFEntityInterface* entity = Cast<IACFEntityInterface>(inTarget);
    SetTargetActorBK(inTarget);
    TargetingComponent->SetCurrentTarget(inTarget);

    if (inTarget && entity && IACFEntityInterface::Execute_IsEntityAlive(inTarget) && UACFFunctionLibrary::AreEnemyTeams(GetWorld(), CombatTeam, IACFEntityInterface::Execute_GetEntityCombatTeam(inTarget))) {
        SetCurrentAIStateBK(EAIState::EBattle);

        // if our target is changed to a new target
        if (!inTarget || inTarget == CharacterOwned) {
            if (currentTarget) {

                UACFDamageHandlerComponent* oldDamageComp = currentTarget->FindComponentByClass<UACFDamageHandlerComponent>();
                if (oldDamageComp) {
                    oldDamageComp->OnOwnerDeath.RemoveDynamic(this, &AACFAIController::OnTargetDeathHandle);
                }
            }
            return;
        }
        UACFDamageHandlerComponent* damageComp = inTarget->FindComponentByClass<UACFDamageHandlerComponent>();
        if (damageComp && !damageComp->OnOwnerDeath.IsAlreadyBound(this, &AACFAIController::OnTargetDeathHandle)) {
            damageComp->OnOwnerDeath.AddDynamic(this, &AACFAIController::OnTargetDeathHandle);
        }

        if (GroupOwner && GroupOwner->GetAlertOtherTeamMembers() && !GroupOwner->IsInBattle()) {
            GroupOwner->SetInBattle(true, inTarget);
        }
    } else {
        ThreatComponent->RemoveThreatening(currentTarget);
        ResetToDefaultState();
    }
}
*/
void AACFAIController::SetTarget(AActor* currentTarget)
{
    AActor* previousTarget = GetTargetActorBK();
    SetTargetActorBK(currentTarget);
    TargetingComponent->SetCurrentTarget(currentTarget);

    if (currentTarget) {
        IACFEntityInterface* entity = Cast<IACFEntityInterface>(currentTarget);
        if (entity && IACFEntityInterface::Execute_IsEntityAlive(currentTarget) && UACFFunctionLibrary::AreEnemyTeams(GetWorld(), CombatTeam, IACFEntityInterface::Execute_GetEntityCombatTeam(currentTarget))) {
            SetCurrentAIStateBK(EAIState::EBattle);

            // Add OnOwnerDeath event for current target.
            UACFDamageHandlerComponent* damageComp = currentTarget->FindComponentByClass<UACFDamageHandlerComponent>();
            if (damageComp && !damageComp->OnOwnerDeath.IsAlreadyBound(this, &AACFAIController::OnTargetDeathHandle)) {
                damageComp->OnOwnerDeath.AddDynamic(this, &AACFAIController::OnTargetDeathHandle);
            }

            // Switching targets. Remove OnOwnerDeath event for previous target.
            if (previousTarget && currentTarget != previousTarget) {
                UACFDamageHandlerComponent* oldDamageComp = previousTarget->FindComponentByClass<UACFDamageHandlerComponent>();
                if (oldDamageComp) {
                    oldDamageComp->OnOwnerDeath.RemoveDynamic(this, &AACFAIController::OnTargetDeathHandle);
                }
                // return
            }

            if (GroupOwner && GroupOwner->GetAlertOtherTeamMembers() && !GroupOwner->IsInBattle()) {
                GroupOwner->SetInBattle(true, currentTarget);
            }

        }
        else {
            // Target is dead or is on the same team, so remove any threat and return AI to default state.
            ThreatComponent->RemoveThreatening(currentTarget);
            ResetToDefaultState();
        }
    }
    else {
        // There is no currentTarget; Remove threat toward previous target.
        if (previousTarget) {
            ThreatComponent->RemoveThreatening(previousTarget);
        }

        // On return home, previous target exists. On return home task completed, previous target will be a nullptr, so put AI back to default state.
        // This prevents AI from ignoring player during return home, if player re-enters AI's sights.
        if (!currentTarget && !previousTarget) {
            ResetToDefaultState();
        }
    }
}

AActor* AACFAIController::GetTarget() const
{
    return TargetingComponent->GetCurrentTarget();
}

bool AACFAIController::HasTarget() const
{
    return TargetingComponent->HasTarget();
}

bool AACFAIController::RequestAnotherTarget()
{
    if (CharacterOwned && IsValid(CharacterOwned)) {
        AActor* target = ThreatComponent->GetActorWithHigherThreat();
        if (GroupOwner && !target) {
            target = GroupOwner->RequestNewTarget(this);
        }

        if (target && target != GetTargetActorBK()) {
            SetTarget(target);
            return true;
        }
    }
    return false;
}

void AACFAIController::HandleCharacterHit(const FACFDamageEvent& damageReceived)
{
    if (!damageReceived.DamageDealer) {
        return;
    }

    if (bShouldReactOnHit) {
        ThreatComponent->AddThreat(damageReceived.DamageDealer, damageReceived.FinalDamage);
        if (GetAIStateBK() != EAIState::EBattle) {
            SetTarget(ThreatComponent->GetActorWithHigherThreat());
        }

        if (GroupOwner && GroupOwner->GetAlertOtherTeamMembers() && !GroupOwner->IsInBattle()) {
            GroupOwner->SetInBattle(true, damageReceived.DamageDealer);
        }
    }
}

void AACFAIController::HandleCharacterDeath()
{
    SetCurrentAIStateBK(EAIState::EWait);
    if (CharacterOwned) {
        CharacterOwned->GetMovementComponent()->StopMovementImmediately();
    }

//     UnPossess();
//     Destroy();
}
