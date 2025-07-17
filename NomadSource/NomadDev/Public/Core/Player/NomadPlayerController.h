// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Interface/MultiplayerInterface.h"
#include "Groups/ACFCompanionsPlayerController.h"
#include "NomadPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
// Forward declarations for UI classes.
class ULobbyMenu;
class UUserWidget;
class UCommonMultiplayerLobbyMenu;
class UCommonActivatableWidget;

/**
 * FPlayerInfo:
 * Structure used to store and replicate lobby player information.
 */
USTRUCT(BlueprintType)
struct FPlayerInfo {
    GENERATED_BODY()

    // Initialize PlayerID to 0 by default.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Sessions Subsystem | Lobby Player Controller | Player Info")
    int32 PlayerID = 0; // A unique ID assigned to the player for tracking.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Sessions Subsystem | Lobby Player Controller | Player Info")
    FName PlayerName = FName("PlayerName"); // The player's display name.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Sessions Subsystem | Lobby Player Controller | Player Info")
    bool bIsReady = false; // Whether the player is marked as "ready" for the game.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Sessions Subsystem | Lobby Player Controller | Player Info")
    ACharacter* PlayerCharacter = nullptr; // Reference to the player's in-game character.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Sessions Subsystem | Lobby Player Controller | Player Info")
    FUniqueNetIdRepl PlayerUniqueNetId; // The player's unique network identifier.

    // Overloaded equality operator to compare two FPlayerInfo objects.
    bool operator==(const FPlayerInfo& Other) const
    {
        // Compare by name and ready status.
        return PlayerName == Other.PlayerName && bIsReady == Other.bIsReady;
    }
};

/**
 * 
 */
UCLASS()
class NOMADDEV_API ANomadPlayerController : public AACFCompanionsPlayerController, public IMultiplayerInterface
{
	GENERATED_BODY()

public:

    ANomadPlayerController();

    virtual void GetSeamlessTravelActorList(bool bToEntry, TArray<class AActor*>& ActorList) override;

    virtual void SetupInputComponent() override;

        // Replicated player info for the lobby.
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Multiplayer Sessions Subsystem | Lobby Player Controller")
    FPlayerInfo PlayerInfo;

    // --- Server RPCs ---

    /**
     * Server_UpdatePlayerList:
     * Instructs the server to update the lobby player list across all clients.
     */
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_UpdatePlayerList(const TArray<FPlayerInfo>& PlayerList);

    /**
     * Server_RequestInitialPlayerList:
     * Called by a client to request the current player list when joining the lobby.
     */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Multiplayer Sessions Subsystem | Lobby Player Controller")
    void Server_RequestInitialPlayerList();

    /**
     * Server_SetPlayerReady:
     * Marks the player as ready on the server.
     */
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetPlayerReady();

    /**
     * Server_SetPlayerNotReady:
     * Marks the player as not ready on the server.
     */
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetPlayerNotReady();

    // --- Client RPCs ---

    /**
     * Client_UpdatePlayerList:
     * Called by the server to update the client with the current lobby player list.
     */
    UFUNCTION(Client, Reliable)
    void Client_UpdatePlayerList(const TArray<FPlayerInfo>& PlayerList);

    // --- Setter Functions ---

    /**
     * SetLobbyMenuWidgetReference:
     * Stores a reference to the lobby menu widget for later UI updates.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | Lobby Player Controller")
    void SetLobbyMenuWidgetReference(UCommonActivatableWidget* InWidget)
    {
        LobbyMenuWidget = InWidget;
    }

protected:
    // -------------------------------------------------------------------------
    // Lifecycle:
    // BeginPlay is used for initial setup.
    // GetLifetimeReplicatedProps ensures that PlayerInfo is replicated.
    // -------------------------------------------------------------------------
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    /** 
     * A function added by Nomad Dev Team 
     * → Called when the player wants to switch which quickbar is “live.”
     *    NewQuickbarIndex should be 0 or 1.
     */
    UFUNCTION(BlueprintCallable, Category = "ACF | Quickbar")
    void ToggleQuickbar();

        // --- UI Widget Classes for Lobby ---

    // Widget class for the host’s lobby menu.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Multiplayer Sessions Subsystem | Lobby Player Controller | UI")
    TSubclassOf<UCommonMultiplayerLobbyMenu> LobbyMenuWidgetClass;

    // Widget class for the client's lobby menu.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Multiplayer Sessions Subsystem | Lobby Player Controller | UI")
    TSubclassOf<UCommonMultiplayerLobbyMenu> LobbyMenuClientWidgetClass;

    // Pointer to the instance of the lobby UI widget.
    UPROPERTY()
    TObjectPtr<UCommonActivatableWidget> LobbyMenuWidget;

    UPROPERTY(EditAnywhere, Category = "Enhanced Input")
    TObjectPtr<UInputMappingContext> InputMappingContext;
};
