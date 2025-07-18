// -----------------------------------------------------------------------------
// UCommonMultiplayerLobbyListEntry.cpp
//
// Implements the functionality for the lobby list entry widget. This widget
// binds to the join session delegate, initiates session joining, and manages UI
// feedback for joining a session.
// -----------------------------------------------------------------------------

#include "Core/MultiplayerSession/Widget/CommonMultiplayerLobbyListEntry.h"
#include "Subsystem/MultiplayerSessionsSubsystem.h"     // To call session functions.
#include "Components/Button.h"                            // For UButton bindings.
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

void UCommonMultiplayerLobbyListEntry::MenuSetup()
{
    // -------------------------------------------------------------------------
    // Retrieve the MultiplayerSessionsSubsystem:
    // Get the subsystem from the GameInstance. This subsystem is responsible for all
    // session operations (join, find, destroy, etc.).
    // -------------------------------------------------------------------------
    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    }

    // -------------------------------------------------------------------------
    // Bind Join Session Callback:
    // Bind the OnJoinSession callback so that when the subsystem completes a join
    // attempt, our function is called. To avoid duplicate bindings, ensure that
    // MenuSetup is only called once per widget instance.
    // -------------------------------------------------------------------------
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionsComplete.AddUObject(this, &ThisClass::OnJoinSession);
    }
}

bool UCommonMultiplayerLobbyListEntry::Initialize()
{
    if (!Super::Initialize())
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Bind Button Click Events:
    // Bind the JoinButton's OnClicked event to our callback.
    // Ensure this is bound only once (Initialize should be called only once per instance).
    // -------------------------------------------------------------------------
    if (JoinButton)
    {
        JoinButton->OnClicked().AddUObject(this, &ThisClass::JoinButtonClicked);
    }

    return true;
}

void UCommonMultiplayerLobbyListEntry::NativeDestruct()
{
    // -------------------------------------------------------------------------
    // Unbind Delegates:
    // Unbind the join session delegate to avoid calling callbacks on a destroyed widget.
    // -------------------------------------------------------------------------
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionsComplete.RemoveAll(this);
    }

    Super::NativeDestruct();
}

void UCommonMultiplayerLobbyListEntry::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
    // -------------------------------------------------------------------------
    // Retrieve the online subsystem to get session details.
    // -------------------------------------------------------------------------
    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    if (Subsystem)
    {
        IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            // Retrieve the connection string using the default session name (NAME_GameSession).
            FString Address;
            SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

            // Get the local player controller using GetFirstLocalPlayerController() to ensure correct ownership.
            //APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
            APlayerController* PlayerController = GetOwningPlayer();
            if (PlayerController)
            {
                // Display a debug message showing the connection string.
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(
                        -1,
                        5.f,
                        FColor::Purple,
                        FString::Printf(TEXT("On Join Session -> Address: %s "), *Address)
                        );
                }

                // Notify Blueprints that joining the session was successful.
                IsJoinSessionSuccessful(true);

                // Initiate client travel to the resolved address.
                PlayerController->ClientTravel(Address, TRAVEL_Absolute);
            }
        }
    }
}

void UCommonMultiplayerLobbyListEntry::OnSessionFailure(const FUniqueNetId& UniqueNetId,
    ESessionFailure::Type SessionFailureType)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
            FString::Printf(TEXT("UCommonMultiplayerLobbyListEntry::OnSessionFailure for player: %s, Failure type: %s"),
                *UniqueNetId.ToString(), LexToString(SessionFailureType)));
    }

    JoinButton->SetIsEnabled(true);
}

void UCommonMultiplayerLobbyListEntry::JoinButtonClicked()
{
    // -------------------------------------------------------------------------
    // Prevent Duplicate Clicks:
    // Disable the join button immediately to avoid triggering the join process multiple times.
    // Optionally, you could use a debounce flag if further logic is required.
    // -------------------------------------------------------------------------
    if (JoinButton)
    {
        JoinButton->SetIsEnabled(false);
    }

    // -------------------------------------------------------------------------
    // Trigger Join Session:
    // Ideally, the subsystem would already be attempting to join when the delegate is fired.
    // If needed, you could explicitly call a join function on the subsystem here.
    // For now, this function simply serves as the UI handler.
    // -------------------------------------------------------------------------
    // (Optional) If you want to initiate join session here, uncomment and call:
    // if (MultiplayerSessionsSubsystem)
    // {
    //     MultiplayerSessionsSubsystem->JoinSession( /* appropriate parameters */ );
    // }
}