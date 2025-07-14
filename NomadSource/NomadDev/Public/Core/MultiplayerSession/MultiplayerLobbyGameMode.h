// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemTypes.h"
#include "GameFramework/GameMode.h"
#include "MultiplayerLobbyGameMode.generated.h"

class ANomadPlayerController;

struct FPlayerInfo;

/**
 * Summary of MultiplayerLobbyGameMode:

    Player Join (PostLogin):
	    When a new player connects, they are added to a list of controllers and their player info is stored.
	    The host is marked as ready by default.
	    The player list is updated and sent to all clients.
    Player List Updates:
	    UpdatePlayerList rebuilds the player info array and pushes the update to all clients.
	    UpdateLobbyList simply iterates through controllers and calls the client RPC to update UI.
    Player Logout:
	    When a player disconnects, they are removed from both the controllers array and the player info array.
	    The lobby list is updated immediately.
 */

/**
 * AMultiplayerLobbyGameMode manages player connections, ready states,
 * and keeps the lobby's player list updated across clients.
 */
UCLASS()
class NOMADDEV_API AMultiplayerLobbyGameMode : public AGameMode {
    GENERATED_BODY()

public:
    AMultiplayerLobbyGameMode();

    virtual void GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList) override;

    // Called when a new player logs in.
    virtual void PostLogin(APlayerController* NewPlayer) override;

    // Called when a player logs out.
    virtual void Logout(AController* Exiting) override;

    virtual void StartPlay() override;

    // Rebuilds and replicates the current player list.
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | Lobby Game Mode")
    void UpdatePlayerList();

    // Called (often via RPCs) to update the lobby list.
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | Lobby Game Mode")
    void UpdateLobbyList();
    void TravelToGameMap(FString PathToGameMap);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Delegate handles to clear later
    FDelegateHandle ParticipantJoinedHandle;
    FDelegateHandle ParticipantLeftHandle;

    // Callback functions for when a participant joins or leaves
    void OnParticipantJoined(FName SessionName, const FUniqueNetId& UniqueId);
    void OnParticipantLeft(FName SessionName, const FUniqueNetId& UniqueId, EOnSessionParticipantLeftReason LeaveReason);

private:
    void RefreshSessionState();
    FTimerHandle RefreshSessionStateTimerHandle;

    // Holds pointers to all connected LobbyPlayerControllers.
    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Lobby Game Mode", meta = (AllowPrivateAccess = "true"))
    TArray<ANomadPlayerController*> ConnectedPlayerControllers;

    // Holds information on all connected players, replicated so all clients see it.
    UPROPERTY(ReplicatedUsing = OnRep_ConnectedPlayerInfo)
    TArray<FPlayerInfo> ConnectedPlayerInfo;

    // Called automatically on clients when ConnectedPlayerInfo changes.
    UFUNCTION()
    void OnRep_ConnectedPlayerInfo();

    // In OnParticipantJoined, OnParticipantLeft, PostLogin, Logout, etc.:
    void ScheduleRefresh();
};