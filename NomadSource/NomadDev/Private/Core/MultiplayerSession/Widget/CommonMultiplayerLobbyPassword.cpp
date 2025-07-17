// -----------------------------------------------------------------------------
// UCommonMultiplayerLobbyPassword.cpp
//
// Implements the functionality for the lobby password widget. This widget 
// handles the join session process (when a password is required) by binding 
// to the join session delegate and then initiating client travel if successful.
// -----------------------------------------------------------------------------

#include "Core/MultiplayerSession/Widget/CommonMultiplayerLobbyPassword.h"
#include "Subsystem/MultiplayerSessionsSubsystem.h"     // To access session functions.
#include "Components/Button.h"                            // For UButton bindings.
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

void UCommonMultiplayerLobbyPassword::MenuSetup()
{
    // -------------------------------------------------------------------------
    // Retrieve the MultiplayerSessionsSubsystem:
    // Get the subsystem from the GameInstance, which manages session operations.
    // -------------------------------------------------------------------------
    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    }

    // -------------------------------------------------------------------------
    // Bind Subsystem Delegates:
    // Bind the join session delegate to our OnJoinSession callback.
    // Also, bind the wrong-password popup delegate so that Blueprint can show a message.
    // Ensure these bindings occur only once per widget instance.
    // -------------------------------------------------------------------------
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionsComplete.AddUObject(this, &ThisClass::OnJoinSession);
        MultiplayerSessionsSubsystem->MultiplayerCallWrongPasswordPopup.AddDynamic(this, &ThisClass::CallWrongPasswordPopup);
    }

    if (JoinButton)
    {
        JoinButton->SetIsEnabled(true);
    }
}

bool UCommonMultiplayerLobbyPassword::Initialize()
{
    if (!Super::Initialize())
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Bind Button Click Event:
    // Bind the JoinButton's OnClicked event to our callback.
    // Disabling the button on click helps prevent duplicate invocations.
    // -------------------------------------------------------------------------
    if (JoinButton)
    {
        JoinButton->OnClicked().AddUObject(this, &ThisClass::JoinButtonClicked);
    }
    return true;
}

void UCommonMultiplayerLobbyPassword::NativeDestruct()
{
    // -------------------------------------------------------------------------
    // Unbind Delegates:
    // Remove all delegate bindings to avoid callbacks on a destroyed widget.
    // -------------------------------------------------------------------------
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionsComplete.RemoveAll(this);
        MultiplayerSessionsSubsystem->MultiplayerCallWrongPasswordPopup.RemoveAll(this);
    }

    Super::NativeDestruct();
}


void UCommonMultiplayerLobbyPassword::OnSessionFailure(const FUniqueNetId& UniqueNetId,
    ESessionFailure::Type SessionFailureType)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
            FString::Printf(TEXT("UCommonMultiplayerLobbyPassword::OnSessionFailure for player: %s, Failure type: %s"),
                *UniqueNetId.ToString(), LexToString(SessionFailureType)));
    }

    if (JoinButton)
    {
        JoinButton->SetIsEnabled(true);
    }
}

void UCommonMultiplayerLobbyPassword::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
    // -------------------------------------------------------------------------
    // Retrieve the online subsystem and session interface.
    // -------------------------------------------------------------------------
    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    if (Subsystem)
    {
        IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            // ---------------------------------------------------------------------
            // Get the connection string for the session using the default session name.
            // ---------------------------------------------------------------------
            FString Address;
            SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

            // ---------------------------------------------------------------------
            // Retrieve the local player controller.
            // Use GetFirstLocalPlayerController to ensure we have a valid local player.
            // ---------------------------------------------------------------------
            APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
            if (PlayerController)
            {
                // Debug: Print the connection string.
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(
                        -1,
                        5.f,
                        FColor::Purple,
                        FString::Printf(TEXT("On Join Session -> Address: %s "), *Address)
                        );
                }
                // Notify Blueprints that the join session operation was successful.
                IsJoinSessionSuccessful(true);
                // Initiate client travel to the resolved address.
                PlayerController->ClientTravel(Address, TRAVEL_Relative, true);
            }
        }
    }
}

void UCommonMultiplayerLobbyPassword::JoinButtonClicked()
{
    // -------------------------------------------------------------------------
    // Prevent Duplicate Button Clicks:
    // Disable the join button immediately so that multiple clicks do not trigger 
    // multiple join attempts.
    // -------------------------------------------------------------------------
    if (JoinButton)
    {
        JoinButton->SetIsEnabled(false);
    }

    // -------------------------------------------------------------------------
    // Optionally, you can directly call a join function on the subsystem here.
    // However, if the join process is already being handled (and OnJoinSession is bound),
    // this function can simply serve as the UI handler.
    // -------------------------------------------------------------------------
    // Example (if needed):
    // if (MultiplayerSessionsSubsystem)
    // {
    //     MultiplayerSessionsSubsystem->JoinSession( /* parameters if required */ );
    // }
}