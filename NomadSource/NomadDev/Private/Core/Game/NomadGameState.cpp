// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/Game/NomadGameState.h"

#include "Core/Player/NomadPlayerController.h"
#include "Net/UnrealNetwork.h"

ANomadGameState::ANomadGameState(): CurrentPlayerCount(0)
{

}

void ANomadGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ANomadGameState, CurrentPlayerCount);
    DOREPLIFETIME(ANomadGameState, ConnectedPlayerInfo);
}

void ANomadGameState::OnRep_CurrentPlayerCount()
{
    // Tell your UI widget to rebuild from ConnectedPlayerInfo
    if (auto* PC = Cast<ANomadPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        PC->Client_UpdatePlayerList(ConnectedPlayerInfo);
    }
}

void ANomadGameState::OnRep_PlayerInfo()
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

