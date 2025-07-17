// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerGameState.generated.h"

struct FPlayerInfo;
/**
 * 
 */
UCLASS()
class NOMADDEV_API AMultiplayerGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AMultiplayerGameState();

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