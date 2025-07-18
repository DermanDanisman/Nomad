// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/Player/NomadPlayerController.h"

#include "CommonActivatableWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Core/MultiplayerSession/MultiplayerLobbyGameMode.h"
#include "Core/MultiplayerSession/Widget/CommonMultiplayerLobbyMenu.h"
#include "Net/UnrealNetwork.h"

ANomadPlayerController::ANomadPlayerController()
{
    bReplicates = true;
}

void ANomadPlayerController::GetSeamlessTravelActorList(bool bToEntry, TArray<class AActor*>& ActorList)
{
    Super::GetSeamlessTravelActorList(bToEntry, ActorList);
}

void ANomadPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
}

void ANomadPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // ✅ CORRECT way to get Enhanced Input subsystem in Player Controller
    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    if (Subsystem)
    {
        Subsystem->AddMappingContext(InputMappingContext, 0);

        // Debug to confirm it worked
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
                TEXT("✅ Input Mapping Context Added Successfully!"));
        }
    }

    FString MapName = GetWorld()->GetMapName();
    if (!MapName.Contains("Lobby"))
    {
        // We're not in the lobby anymore. Destroy the lobby widget.
        if (LobbyMenuWidget)
        {
            LobbyMenuWidget->RemoveFromParent();
            LobbyMenuWidget = nullptr;
        }
    }

    if (IsLocalController() && MapName.Contains("Lobby"))
    {
        Server_RequestInitialPlayerList();
    }
}

void ANomadPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // Replicate the PlayerInfo structure to synchronize lobby state across clients.
    DOREPLIFETIME(ANomadPlayerController, PlayerInfo);
}

// ------------------ Server RPCs ------------------

// Server_UpdatePlayerList:
// Called on the server to update all clients' player lists.
void ANomadPlayerController::Server_UpdatePlayerList_Implementation(const TArray<FPlayerInfo>& PlayerList)
{
    if (HasAuthority())
    {
        // Broadcast the new player list to all clients.
        Client_UpdatePlayerList(PlayerList);
    }
}

bool ANomadPlayerController::Server_UpdatePlayerList_Validate(const TArray<FPlayerInfo>& PlayerList)
{
    return true;
}

// Server_RequestInitialPlayerList:
// Called by a client to request the current lobby player list.
void ANomadPlayerController::Server_RequestInitialPlayerList_Implementation()
{
    // Ask the game mode to update and send the current player list.
    if (AMultiplayerLobbyGameMode* LobbyGameMode = Cast<AMultiplayerLobbyGameMode>(GetWorld()->GetAuthGameMode()))
    {
        LobbyGameMode->UpdatePlayerList();
    }
}

bool ANomadPlayerController::Server_RequestInitialPlayerList_Validate()
{
    return true;
}

// Server_SetPlayerReady:
// Marks the player as ready on the server, then requests a lobby update.
void ANomadPlayerController::Server_SetPlayerReady_Implementation()
{
    if (HasAuthority())
    {
        PlayerInfo.bIsReady = true;
        if (AMultiplayerLobbyGameMode* LobbyGameMode = Cast<AMultiplayerLobbyGameMode>(GetWorld()->GetAuthGameMode()))
        {
            LobbyGameMode->UpdatePlayerList();
        }
    }
}

bool ANomadPlayerController::Server_SetPlayerReady_Validate()
{
    return true;
}

// Server_SetPlayerNotReady:
// Marks the player as not ready on the server, then requests a lobby update.
void ANomadPlayerController::Server_SetPlayerNotReady_Implementation()
{
    if (HasAuthority())
    {
        PlayerInfo.bIsReady = false;
        if (AMultiplayerLobbyGameMode* LobbyGameMode = Cast<AMultiplayerLobbyGameMode>(GetWorld()->GetAuthGameMode()))
        {
            LobbyGameMode->UpdatePlayerList();
        }
    }
}

bool ANomadPlayerController::Server_SetPlayerNotReady_Validate()
{
    return true;
}

// ------------------ Client RPCs ------------------

// Client_UpdatePlayerList:
// Called on the client to update the lobby UI with the current player list.
void ANomadPlayerController::Client_UpdatePlayerList_Implementation(const TArray<FPlayerInfo>& PlayerList)
{
    // For debugging, iterate over the player list and print each player's details.
    if (GEngine)
    {
        for (int32 Index = 0; Index < PlayerList.Num(); ++Index)
        {
            const FPlayerInfo& Info = PlayerList[Index];
            FString UniqueIdStr;
            // Safely retrieve the player's unique net ID as a string.
            TSharedPtr<const FUniqueNetId> UniqueIdPtr = Info.PlayerUniqueNetId.GetUniqueNetId();
            UniqueIdStr = UniqueIdPtr.IsValid() ? UniqueIdPtr->ToString() : TEXT("Invalid UniqueId");

            FString DebugMessage = FString::Printf(TEXT("PlayerID: %d | Name: %s | Ready: %s | UniqueId: %s"),
                Info.PlayerID,
                *Info.PlayerName.ToString(),
                (Info.bIsReady ? TEXT("Yes") : TEXT("No")),
                *UniqueIdStr);
            GEngine->AddOnScreenDebugMessage(Index, 5.f, FColor::Green, DebugMessage);
            UE_LOG(LogTemp, Log, TEXT("%s"), *DebugMessage);
        }
    }

    // If the lobby menu widget exists, pass the updated player list to update the UI.
    if (LobbyMenuWidget)
    {
        if (UCommonMultiplayerLobbyMenu* CastedLobbyMenu = Cast<UCommonMultiplayerLobbyMenu>(LobbyMenuWidget))
        {
            CastedLobbyMenu->UpdatePlayerList(PlayerList);
        }
    }
}

/**
 * ToggleQuickbar
 */
void ANomadPlayerController::ToggleQuickbar()
{
    UE_LOG(LogTemp, Warning, TEXT("[PC] ToggleQuickbar called"));

    // Get the controlled character first
    AACFCharacter* ControlledCharacter = Cast<AACFCharacter>(GetPawn());
    if (!ControlledCharacter) return;

    // Get the EquipmentComp from the character
    UACFEquipmentComponent* EquipmentComp = ControlledCharacter->GetEquipmentComponent();
    if (!EquipmentComp) return;

    // Debug: Print current state on screen
    const EActiveQuickbar CurrentBar = EquipmentComp->GetActiveQuickbarEnum();

    // Toggle between Combat and Tools
    const EActiveQuickbar Next =
        (CurrentBar == EActiveQuickbar::Combat)
          ? EActiveQuickbar::Tools
          : EActiveQuickbar::Combat;

    // Try the switch with additional error checking
    if (EquipmentComp->IsValidLowLevel())
    {
        EquipmentComp->SetActiveQuickbarEnum(Next);
    }
}