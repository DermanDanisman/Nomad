// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/ACFGameState.h"
#include "NomadGameState.generated.h"

struct FPlayerInfo;
/**
 * Nomad game state class
 * Manages multiplayer game state including player count and player information
 */
UCLASS()
class NOMADDEV_API ANomadGameState : public AACFGameState
{
	GENERATED_BODY()
public:

    ANomadGameState();

    // This property will be replicated to all clients.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player Count")
    int32 CurrentPlayerCount;

    // Optionally, you can add an OnRep function to handle updates on clients.
    UFUNCTION()
    void OnRep_CurrentPlayerCount();

    // in AMultiplayerLobbyGameState.h
    UPROPERTY(ReplicatedUsing=OnRep_PlayerInfo)
    TArray<FPlayerInfo> ConnectedPlayerInfo;

    UFUNCTION()
    void OnRep_PlayerInfo();

    // Required for replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};
