// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#include "Game/ACFGameState.h"
#include "ACMEffectsDispatcherComponent.h"
#include "AIController.h"
#include "AQSQuestFunctionLibrary.h"
#include "AQSQuestManagerComponent.h"
#include "Components/ACFTeamManagerComponent.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

void AACFGameState::UpdateBattleState()
{
	const EBattleState state = InBattleAIs.Num() > 0 ? EBattleState::EBattle : EBattleState::EExploration;
	if (battleState != state)
	{
		battleState = state;
		OnBattleStateChanged.Broadcast(battleState);
	}
}

AACFGameState::AACFGameState()
{
	EffectsComp = CreateDefaultSubobject<UACMEffectsDispatcherComponent>(TEXT("Effects Component"));
	TeamManagerComponent = CreateDefaultSubobject<UACFTeamManagerComponent>(TEXT("Team Manager"));
        PlayerCount = 0;
}

void AACFGameState::AddAIToBattle(AAIController* contr)
{
	if (!contr) {
		return;
	}
		
	InBattleAIs.Add(contr);
	UpdateBattleState();
}

void AACFGameState::RemoveAIFromBattle(AAIController* contr)
{
	if (contr && InBattleAIs.Contains(contr))
	{
		InBattleAIs.Remove(contr);
		UpdateBattleState();
	}
}
void AACFGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AACFGameState, PlayerCount);
}

int32 AACFGameState::GetPlayerCount() const
{
    return PlayerCount;
}

void AACFGameState::SetPlayerCount(int32 count)
{
    PlayerCount = count;
}

void AACFGameState::UpdatePlayersObjectivesRepetitions(FGameplayTag Objective, FGameplayTag Quest)
{
    // Ensure there is at least one player in the PlayerArray
    if (PlayerArray.Num() == 0 || !IsValid(PlayerArray[0]))
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid players found in PlayerArray."));
        return;
    }

    // Get the first player's owner (used to read the current repetitions from their quest)
    APlayerState* FirstPlayerState = PlayerArray[0];
    AActor* PlayerStateOwner = FirstPlayerState->GetOwner();

    if (!IsValid(PlayerStateOwner))
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerStateOwner is not valid."));
        return;
    }

    // Get the QuestManager
    UAQSQuestManagerComponent* QuestManager = Cast<UAQSQuestManagerComponent>(GetComponentByClass(UAQSQuestManagerComponent::StaticClass())
    );

    if (!IsValid(QuestManager))
    {
        UE_LOG(LogTemp, Warning, TEXT("QuestManager component not found on player state owner."));
        return;
    }

    // Check if the specified objective is in progress
    if (!QuestManager->IsObjectiveInProgress(Objective))
    {
        UE_LOG(LogTemp, Warning, TEXT("Objective is not in progress."));
        return;
    }

    // Get the current and max repetitions from the first player's quest manager
    UAQSQuest* QuestInstance = QuestManager->GetQuest(Quest);
    if (!IsValid(QuestInstance))
    {
        UE_LOG(LogTemp, Warning, TEXT("Quest not found on QuestManager."));
        return;
    }

    UAQSQuestObjective* ObjectiveInstance = QuestInstance->GetObjectiveByTag(Objective);
    if (!IsValid(ObjectiveInstance))
    {
        UE_LOG(LogTemp, Warning, TEXT("Objective not found on Quest."));
        return;
    }

    int CurrentRepetitions = ObjectiveInstance->GetCurrentRepetitions();
    int MaxRepetitions = ObjectiveInstance->GetRepetitions();

    // Only proceed if the objective hasn't reached the maximum repetition count
    if (CurrentRepetitions >= MaxRepetitions)
        return;

    // Loop through all player states to synchronize their objective repetitions
    for (APlayerState* PlayerState : PlayerArray)
    {
        if (!IsValid(PlayerState))
            continue;

        AActor* PSOwner = PlayerState->GetOwner();
        if (!IsValid(PSOwner))
            continue;

        // Get the quest manager component for each player's owner
        UAQSQuestManagerComponent* PlayerQuestManager = Cast<UAQSQuestManagerComponent>(
            PSOwner->GetComponentByClass(UAQSQuestManagerComponent::StaticClass())
        );

        if (!IsValid(PlayerQuestManager))
            continue;

        UAQSQuest* PlayerQuest = PlayerQuestManager->GetQuest(Quest);
        if (!IsValid(PlayerQuest))
            continue;

        UAQSQuestObjective* PlayerObjective = PlayerQuest->GetObjectiveByTag(Objective);
        if (!IsValid(PlayerObjective))
            continue;

        // Update the objective's current repetitions if not equal to the synced value
        if (PlayerObjective->GetCurrentRepetitions() != CurrentRepetitions)
        {
            PlayerObjective->SetCurrentRepetitions(CurrentRepetitions);
        }
    }
}
