// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/MultiplayerSession/MultiplayerGameState.h"

#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Core/Player/NomadPlayerController.h"

AMultiplayerGameState::AMultiplayerGameState(): CurrentPlayerCount(0)
{

}

void AMultiplayerGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(AMultiplayerGameState, ConnectedPlayerInfo, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(AMultiplayerGameState, CurrentPlayerCount, COND_None, REPNOTIFY_Always);
}

void AMultiplayerGameState::OnRep_PlayerInfo()
{
    // Tell your UI widget to rebuild from ConnectedPlayerInfo
    if (auto* PC = Cast<ANomadPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        PC->Client_UpdatePlayerList(ConnectedPlayerInfo);
    }
}

void AMultiplayerGameState::OnRep_CurrentPlayerCount()
{
    // Optionally, notify UI or do something when the count updates.
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            15.f,
            FColor::Purple,
            FString::Printf(TEXT("OnRep_CurrentPlayerCount: %d"), CurrentPlayerCount)
            );
    }
}