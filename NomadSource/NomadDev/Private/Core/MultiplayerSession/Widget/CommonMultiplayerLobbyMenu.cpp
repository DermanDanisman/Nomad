// -----------------------------------------------------------------------------
// UCommonMultiplayerLobbyMenu.cpp
//
// Implementation of the lobby menu widget that manages the multiplayer lobby UI.
// Handles session start, destruction, and end callbacks, button clicks, and UI updates.
// -----------------------------------------------------------------------------


#include "Core/MultiplayerSession/Widget/CommonMultiplayerLobbyMenu.h"

#include "Subsystem/MultiplayerSessionsSubsystem.h"
#include "Components/Button.h"
#include "CommonButtonBase.h"
#include "Core/MultiplayerSession/MultiplayerLobbyGameMode.h"
#include "Core/Player/NomadPlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Subsystem/MultiplayerMapPathSubsystem.h"

void UCommonMultiplayerLobbyMenu::NativeOnActivated()
{
    Super::NativeOnActivated();
    // Optionally, add any activation-specific logic here (e.g., reset button states).

}

void UCommonMultiplayerLobbyMenu::MenuSetup(bool bIsInLobby)
{

    // Cache the current world pointer.
    CurrentWorld = GetWorld();
    if (GEngine && CurrentWorld)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple,
            FString::Printf(TEXT("Menu Setup: World is: %s"), *CurrentWorld->GetName()));
    }

    // Retrieve the MultiplayerSessionsSubsystem from the GameInstance.
    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
        const UMultiplayerMapPathSubsystem* MapPathSubsystem = GetGameInstance()->GetSubsystem<UMultiplayerMapPathSubsystem>();
        if (MapPathSubsystem)
        {
            if (MapPathSubsystem->MapPathsDataAsset)
            {
                // Format the lobby and game paths for server travel.
                PathToGame = FString::Printf(TEXT("%s?listen"), *MapPathSubsystem->GetGameMapPath());
            }
        }
    }

    // Bind callbacks for session events from the subsystem.
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
        MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
        MultiplayerSessionsSubsystem->MultiplayerOnEndSessionComplete.AddDynamic(this, &ThisClass::OnEndSession);
        MultiplayerSessionsSubsystem->MultiplayerOnStartSessionActionCompleted.AddUObject(this, &ThisClass::OnStartSessionActionCompleted);
        MultiplayerSessionsSubsystem->MultiplayerOnSessionFailure.AddUObject(this, &UCommonMultiplayerLobbyMenu::OnSessionFailure);
    }

    // Adjust UI elements based on lobby mode.
    if (!bIsInLobby)
    {
        // In non-lobby mode, hide the Start button.
        StartButton->SetVisibility(ESlateVisibility::Collapsed);
    } else
    {
        // In lobby mode, get the first local player controller to set input mode.
        // Use GetFirstLocalPlayerController for correct widget ownership.
        PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
        if (PlayerController)
        {
            // If the controller has authority (host), show the Start button and hide the Ready button.
            if (StartButton && PlayerController->HasAuthority())
            {
                ReadyButton->SetVisibility(ESlateVisibility::Collapsed);
                StartButton->SetVisibility(ESlateVisibility::Visible);
            } else // For clients, show the Ready button and hide the Start button.
            {
                ReadyButton->SetVisibility(ESlateVisibility::Visible);
                StartButton->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }
    // Always show the Close Lobby button.
    CloseLobbyButton->SetVisibility(ESlateVisibility::Visible);
}

bool UCommonMultiplayerLobbyMenu::Initialize()
{
    if (!Super::Initialize())
    {
        return false;
    }

    // Bind button click events. To avoid multiple bindings, ensure Initialize is called only once per widget instance.
    if (CloseLobbyButton)
    {
        CloseLobbyButton->OnClicked().AddUObject(this, &ThisClass::CloseLobbyButtonClicked);
    }
    if (ReadyButton)
    {
        ReadyButton->OnClicked().AddUObject(this, &ThisClass::ReadyButtonClicked);
    }
    if (StartButton)
    {
        StartButton->OnClicked().AddUObject(this, &ThisClass::StartButtonClicked);
    }
    return true;
}

void UCommonMultiplayerLobbyMenu::NativeDestruct()
{
    // Unbind all delegates to prevent callbacks to a destroyed widget and avoid duplicate calls.
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.RemoveAll(this);
        MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveAll(this);
        MultiplayerSessionsSubsystem->MultiplayerOnEndSessionComplete.RemoveAll(this);
        MultiplayerSessionsSubsystem->MultiplayerOnStartSessionActionCompleted.RemoveAll(this);
        MultiplayerSessionsSubsystem->MultiplayerOnSessionFailure.RemoveAll(this);
    }

    MenuTearDown();

    Super::NativeDestruct();
}

// -----------------------------------------------------------------------------
// OnStartSession:
// Callback for when a session start attempt completes.
// If successful, logs the result and travels to the game map.
// -----------------------------------------------------------------------------
void UCommonMultiplayerLobbyMenu::OnStartSession(bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Session started successfully"));
        }

        UWorld* World = GetWorld();
        if (World)
        {
            // Only the server GameMode should initiate travel
            if (AMultiplayerLobbyGameMode* GM = World->GetAuthGameMode<AMultiplayerLobbyGameMode>())
            {
                GM->TravelToGameMap(PathToGame);
            }
        }
    }
    else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to start session"));
        }
    }
}

// -----------------------------------------------------------------------------
// OnDestroySession:
// Callback for when a session destruction attempt completes.
// If destruction fails, re-enable the CloseLobbyButton. Also, return players to the main menu.
// -----------------------------------------------------------------------------
void UCommonMultiplayerLobbyMenu::OnDestroySession(bool bWasSuccessful)
{
    if (!bWasSuccessful)
    {
        if (CloseLobbyButton)
        {
            CloseLobbyButton->SetIsEnabled(true);
        }
    }

    if (CurrentWorld)
    {
        // Ensure we use the local player controller for proper widget context.
        PlayerController = (PlayerController == nullptr) ? *GetGameInstance()->GetFirstLocalPlayerController() : PlayerController;
        if (PlayerController && PlayerController->HasAuthority())
        {
            // For the host, return all clients to the main menu.
            if (AGameModeBase* GameMode = CurrentWorld->GetAuthGameMode<AGameModeBase>())
            {
                GameMode->ReturnToMainMenuHost();
            }
        } else
        {
            // For clients, instruct the LobbyPlayerController to return to the main menu with a message.
            if (ANomadPlayerController* LobbyController = Cast<ANomadPlayerController>(PlayerController))
            {
                LobbyController->ClientReturnToMainMenuWithTextReason(FText::FromString(TEXT("Player left the lobby.")));
            }
        }
    }

    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple,
                FString::Printf(TEXT("UCommonMultiplayerLobbyMenu::OnDestroySession: On Destroy Session -> Successful")));
        }
    } else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple,
                FString::Printf(TEXT("UCommonMultiplayerLobbyMenu::OnDestroySession: On Destroy Session -> Failed")));
        }
    }
}

// -----------------------------------------------------------------------------
// OnEndSession:
// Callback for when a session end event occurs. Currently, logs the event.
// -----------------------------------------------------------------------------
void UCommonMultiplayerLobbyMenu::OnEndSession(bool bWasSuccessful)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Blue,
            FString::Printf(TEXT("UCommonMultiplayerLobbyMenu::OnEndSession: bWasSuccessful: %d"), bWasSuccessful));
    }
}


void UCommonMultiplayerLobbyMenu::OnSessionFailure(const FUniqueNetId& UniqueNetId, ESessionFailure::Type SessionFailureType)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
            FString::Printf(TEXT("UCommonMultiplayerLobbyMenu::OnSessionFailure for player: %s, Failure type: %s"),
                *UniqueNetId.ToString(), LexToString(SessionFailureType)));
    }

    StartButton->SetIsEnabled(true);
}

// -----------------------------------------------------------------------------
// OnStartSessionActionCompleted:
// Called when the session start action completes. Enables or disables the Start button.
// -----------------------------------------------------------------------------
void UCommonMultiplayerLobbyMenu::OnStartSessionActionCompleted(bool bWasSuccessful)
{
    StartButton->SetIsEnabled(bWasSuccessful);
}

// -----------------------------------------------------------------------------
// CloseLobbyButtonClicked:
// Called when the Close Lobby button is pressed. Disables the button immediately to prevent duplicate calls,
// then requests session destruction from the subsystem.
// -----------------------------------------------------------------------------
void UCommonMultiplayerLobbyMenu::CloseLobbyButtonClicked()
{
    CloseLobbyButton->SetIsEnabled(false);

    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->DestroySession();
    }
}

// -----------------------------------------------------------------------------
// ReadyButtonClicked:
// Triggered when the Ready button is pressed. Toggles the player's ready status
// by calling the appropriate server function on the LobbyPlayerController.
// -----------------------------------------------------------------------------
void UCommonMultiplayerLobbyMenu::ReadyButtonClicked()
{
    // Use a cast to ensure we have a LobbyPlayerController.
    if (ANomadPlayerController* LobbyController = Cast<ANomadPlayerController>(PlayerController))
    {
        // Toggle the ready state.
        if (LobbyController->PlayerInfo.bIsReady)
        {
            LobbyController->Server_SetPlayerNotReady();
        } else
        {
            LobbyController->Server_SetPlayerReady();
        }
    }
}

// -----------------------------------------------------------------------------
// GetIsPlayerReadyStatus:
// Returns whether the local player is marked as ready (used for UI updates).
// -----------------------------------------------------------------------------
bool UCommonMultiplayerLobbyMenu::GetIsPlayerReadyStatus()
{
    if (ANomadPlayerController* LobbyController = Cast<ANomadPlayerController>(PlayerController))
    {
        return LobbyController->PlayerInfo.bIsReady;
    }
    return false;
}

// -----------------------------------------------------------------------------
// StartButtonClicked:
// Called when the Start button is pressed by the host.
// Prevents multiple clicks by ensuring the subsystem is only called once.
// -----------------------------------------------------------------------------
void UCommonMultiplayerLobbyMenu::StartButtonClicked()
{
    // Disable the button immediately to avoid multiple invocations.
    StartButton->SetIsEnabled(false);

    // Make 100% sure only the host ever calls StartSession()
    if (PlayerController && PlayerController->HasAuthority())
    {
        if (MultiplayerSessionsSubsystem)
        {
            MultiplayerSessionsSubsystem->StartSession();
        }
    }
    else
    {
        // (Optional) Put up a tooltip or log so you know a client tried.
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1, 5.f, FColor::Yellow,
                TEXT("Only the host can Start the session.")
            );
        }
    }
}

void UCommonMultiplayerLobbyMenu::MenuTearDown()
{
    PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
    if (PlayerController)
    {
        const FInputModeGameOnly InputModeData;
        PlayerController->SetInputMode(InputModeData);
        PlayerController->SetShowMouseCursor(false);
    }
}