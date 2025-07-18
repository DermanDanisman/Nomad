// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/MultiplayerSession/MultiplayerLobbyGameMode.h"
#include "Core/MultiplayerSession/MultiplayerGameState.h"
#include "Subsystem/MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "OnlineSubsystemTypes.h"
#include "Core/Game/NomadGameState.h"
#include "Core/Player/NomadPlayerController.h"
#include "Engine/Engine.h"
#include "GameFramework/GameSession.h"


AMultiplayerLobbyGameMode::AMultiplayerLobbyGameMode()
{
    bUseSeamlessTravel = true;
}

void AMultiplayerLobbyGameMode::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
    Super::GetSeamlessTravelActorList(bToTransition, ActorList);

    for (auto Actor : ActorList)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(1, 30.f, FColor::Purple,
                FString::Printf(TEXT("Multiplayer Lobby Game Mode: Seamless Travel Actor List: %s"), *Actor->GetName()));
        }
    }
}

// ------------------ Initialization & Delegate Binding ------------------

// Called when the game mode begins play
void AMultiplayerLobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (GetWorld())
    {
        // Retrieve the online session interface from the subsystem
        IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
        if (Subsystem)
        {
            IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
            if (SessionInterface.IsValid())
            {
                // Bind the delegate for when a participant joins the session.
                ParticipantJoinedHandle = SessionInterface->AddOnSessionParticipantJoinedDelegate_Handle(
                    FOnSessionParticipantJoinedDelegate::CreateUObject(this, &AMultiplayerLobbyGameMode::OnParticipantJoined)
                    );

                // Bind the delegate for when a participant leaves the session.
                ParticipantLeftHandle = SessionInterface->AddOnSessionParticipantLeftDelegate_Handle(
                    FOnSessionParticipantLeftDelegate::CreateUObject(this, &AMultiplayerLobbyGameMode::OnParticipantLeft)
                    );
            }
        }
    }
}

// Called when the game mode is ending play (for cleanup)
void AMultiplayerLobbyGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Retrieve the session interface again to clear delegates
    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    if (Subsystem)
    {
        IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            // Clear the bound participant join and left delegates
            SessionInterface->ClearOnSessionParticipantJoinedDelegate_Handle(ParticipantJoinedHandle);
            SessionInterface->ClearOnSessionParticipantLeftDelegate_Handle(ParticipantLeftHandle);
        }
    }
    Super::EndPlay(EndPlayReason);
}

// ------------------ Participant Change Callbacks ------------------

// Callback when a participant joins the session
void AMultiplayerLobbyGameMode::OnParticipantJoined(FName SessionName, const FUniqueNetId& UniqueId)
{
    // Calculate the current number of players (using GameState's player array)
    ScheduleRefresh();
}

// Callback when a participant leaves the session
void AMultiplayerLobbyGameMode::OnParticipantLeft(FName SessionName, const FUniqueNetId& UniqueId,
    EOnSessionParticipantLeftReason LeaveReason)
{
    // Update the session’s advertised player count when a player leaves.
    ScheduleRefresh();
}

// Registers properties for replication.
void AMultiplayerLobbyGameMode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // Replicate the ConnectedPlayerInfo array.
    DOREPLIFETIME_CONDITION_NOTIFY(AMultiplayerLobbyGameMode, ConnectedPlayerInfo, COND_None, REPNOTIFY_Always);
}

static int32 NextPlayerID = 1;

// ------------------ Player Connection Management ------------------

// Called when a new player joins the lobby.
void AMultiplayerLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // Ensure GameState and NewPlayer are valid
    if (!GameState || NewPlayer == nullptr)
    {
        return;
    }

    // Cast the new player to our custom LobbyPlayerController.
    ANomadPlayerController* LobbyController = Cast<ANomadPlayerController>(NewPlayer);
    if (!LobbyController)
    {
        return;
    }

    // Retrieve the UniqueNetId from the player's PlayerState.
    const FUniqueNetIdRepl NewPlayerUniqueId = LobbyController->PlayerState ? LobbyController->PlayerState->GetUniqueId() : nullptr;
    if (!NewPlayerUniqueId.IsValid())
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(1, 30.f, FColor::Red,
                FString::Printf(TEXT("PostLogin: NewPlayer's UniqueNetId is not valid.")));
        }
        return;
    }

    // Check for duplicate UniqueNetIds in the current lobby list.
    for (const FPlayerInfo& ExistingInfo : ConnectedPlayerInfo)
    {
        // Ensure the existing info has a valid UniqueNetId.
        if (ExistingInfo.PlayerUniqueNetId.IsValid())
        {
            // Compare by converting to string. Alternatively, use a custom comparator if available.
            if (ExistingInfo.PlayerUniqueNetId == NewPlayerUniqueId)
            {
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(1, 30.f, FColor::Red,
                        FString::Printf(TEXT("Duplicate connection detected!")));
                }

                // Optionally, kick the duplicate. For example, you might call:
                // KickPlayerFromLobby(*NewPlayerUniqueId);
                // Or force the duplicate to return to the main menu.
                // Here, we simply log and return early.
                return;
            }
        }
    }

    // Add the controller and its player info to local arrays
    ConnectedPlayerControllers.Add(LobbyController);
    ConnectedPlayerInfo.Add(LobbyController->PlayerInfo);

    // For the host (local controller), set them as ready.
    if (LobbyController->IsLocalPlayerController())
    {
        LobbyController->PlayerInfo.bIsReady = true;
        LobbyController->PlayerInfo.PlayerName = FName(LobbyController->GetPlayerState<APlayerState>()->GetPlayerName());
        LobbyController->PlayerInfo.PlayerID = NextPlayerID++;
        LobbyController->PlayerInfo.PlayerUniqueNetId = LobbyController->PlayerState->GetUniqueId();
    } else // For clients, start as not ready.
    {
        LobbyController->PlayerInfo.bIsReady = false;
        LobbyController->PlayerInfo.PlayerName = FName(LobbyController->GetPlayerState<APlayerState>()->GetPlayerName());
        LobbyController->PlayerInfo.PlayerID = NextPlayerID++;
        LobbyController->PlayerInfo.PlayerUniqueNetId = LobbyController->PlayerState->GetUniqueId();
    };

    // Optionally refresh session state (e.g., update UI, propagate changes to the session subsystem).
    ScheduleRefresh();
}

// ------------------ Player Disconnection Management ------------------

// Called when a player logs out.
void AMultiplayerLobbyGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    // If the exiting controller is our custom lobby controller, remove it from arrays
    if (ANomadPlayerController* ExitingController = Cast<ANomadPlayerController>(Exiting))
    {
        int32 PlayerIDToRemove = ExitingController->PlayerInfo.PlayerID;
        ConnectedPlayerControllers.Remove(ExitingController);

        // Remove the player's info from the ConnectedPlayerInfo array.
        for (int32 i = 0; i < ConnectedPlayerInfo.Num(); i++)
        {
            if (ConnectedPlayerInfo[i].PlayerID == PlayerIDToRemove)
            {
                ConnectedPlayerInfo.RemoveAt(i);
                break; // Exit after removal.
            }
        }

        int32 CurrentPlayerCount = GameState->PlayerArray.Num();
        if (CurrentPlayerCount > 1)
        {
            ScheduleRefresh();
        } else if (HasAuthority()) // Ensure only the host executes this block
        {
            // If only the host remains, end the session.
            UMultiplayerSessionsSubsystem* SessionSubsystem = GetGameInstance()->GetSubsystem<UMultiplayerSessionsSubsystem>();
            if (SessionSubsystem)
            {
                GetWorldTimerManager().ClearTimer(RefreshSessionStateTimerHandle);
                SessionSubsystem->EndSession();
            }
        } else // Clients should reset their session interface and return to the main menu.
        {
            GetWorldTimerManager().ClearTimer(RefreshSessionStateTimerHandle);

            // Return to the main menu for clients
            ANomadPlayerController* LobbyController = Cast<ANomadPlayerController>(GetWorld()->GetFirstPlayerController());
            if (LobbyController)
            {
                LobbyController->ClientReturnToMainMenuWithTextReason(FText::FromString(TEXT("Player left the lobby.")));
            }
        }
        ScheduleRefresh();
    }
}

void AMultiplayerLobbyGameMode::StartPlay()
{
    Super::StartPlay();
}

// Helper function to refresh the session state (updates session settings and UI)
void AMultiplayerLobbyGameMode::RefreshSessionState()
{
    if (!GameState)
    {
        return;
    }

    // Set a short timer (0.5 seconds) so that GameState->PlayerArray updates before refreshing session data
    //GetWorldTimerManager().SetTimer(RefreshSessionStateTimerHandle, [this]()
    //{
    // Update the current player count on the server.
    if (ANomadGameState* MultiplayerGameState = GetGameState<ANomadGameState>())
    {
        MultiplayerGameState->CurrentPlayerCount = ConnectedPlayerControllers.Num(); //GameState->PlayerArray.Num();
        UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem = GetGameInstance()->GetSubsystem<UMultiplayerSessionsSubsystem>();
        if (MultiplayerSessionsSubsystem)
        {
            MultiplayerSessionsSubsystem->SetCurrentPlayerCount(MultiplayerGameState->CurrentPlayerCount);
        }
    }
    // Update the lobby list immediately after removal.
    UpdatePlayerList();
    //}, 0.5f, false);
}

// ------------------ Updating the Lobby List ------------------
// Rebuilds the ConnectedPlayerInfo array and sends it to clients.
void AMultiplayerLobbyGameMode::UpdatePlayerList()
{
    TArray<FPlayerInfo> UpdatedList;
    for (ANomadPlayerController* LobbyController : ConnectedPlayerControllers)
    {
        if (LobbyController && LobbyController->PlayerState)
        {
            UpdatedList.Add(LobbyController->PlayerInfo);
        }
    }

    // Update the replicated array.
    ConnectedPlayerInfo = UpdatedList;

    // 1) Store it on GameState (this will replicate it to all clients)
    if (auto* GS = GetGameState<ANomadGameState>())
    {
        GS->ConnectedPlayerInfo = UpdatedList;
    }

    // Broadcast the new player list to each client.\
    // Send the new player list to each client via a client RPC
    for (ANomadPlayerController* LobbyController : ConnectedPlayerControllers)
    {
        if (LobbyController)
        {
            LobbyController->Client_UpdatePlayerList(ConnectedPlayerInfo);
        }
    }

    // 2) Also immediately update the host's UI, since OnRep won't fire on server
    if (auto* HostPC = Cast<ANomadPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        HostPC->Client_UpdatePlayerList(UpdatedList);
    }
}

// Called on clients when the replicated ConnectedPlayerInfo changes.
void AMultiplayerLobbyGameMode::OnRep_ConnectedPlayerInfo()
{
    // Update each client's UI.
    UpdateLobbyList();
}

void AMultiplayerLobbyGameMode::ScheduleRefresh()
{
    GetWorld()->GetTimerManager().ClearTimer(RefreshSessionStateTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        RefreshSessionStateTimerHandle,
        this,
        &AMultiplayerLobbyGameMode::RefreshSessionState,
        0.5f, // debounce delay
        false
    );
}


// Sends the current ConnectedPlayerInfo to each client.
void AMultiplayerLobbyGameMode::UpdateLobbyList()
{
    // Loop through each connected player and send the updated list.
    for (ANomadPlayerController* LobbyController : ConnectedPlayerControllers)
    {
        if (LobbyController)
        {
            LobbyController->Client_UpdatePlayerList(ConnectedPlayerInfo);
        }
    }
}

void AMultiplayerLobbyGameMode::TravelToGameMap(FString PathToGameMap)
{
    // Relative travel preserves "?listen"
    const FString GameMapURL = PathToGameMap; // e.g. "/Game/Maps/MyLobby?listen"
    GetWorld()->ServerTravel(GameMapURL, /*bAbsolute=*/false);
}
