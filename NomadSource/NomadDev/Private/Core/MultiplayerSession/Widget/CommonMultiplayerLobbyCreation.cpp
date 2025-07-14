// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/MultiplayerSession/Widget/CommonMultiplayerLobbyCreation.h"

#include "Subsystem/MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Components/Button.h"
#include "CommonButtonBase.h"
#include "Core/MultiplayerSession/MultiplayerLobbyGameMode.h"
#include "Core/MultiplayerSession/MultiplayerMenuGameMode.h"
#include "Subsystem/MultiplayerMapPathSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

void UCommonMultiplayerLobbyCreation::NativeOnActivated()
{
    Super::NativeOnActivated();
}

// -----------------------------------------------------------------------------
// MenuSetup:
// Configures the session parameters, retrieves the session subsystem, binds delegates,
// and sets button visibility based on whether we are in the lobby.
// -----------------------------------------------------------------------------
void UCommonMultiplayerLobbyCreation::MenuSetup(int32 NumberOfPublicConnections, FString InSessionName, bool bIsInLobby)
{
    // -------------------------------------------------------------------------
    // Retrieve the MultiplayerSessionsSubsystem from the GameInstance.
    // -------------------------------------------------------------------------
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
                PathToLobby = FString::Printf(TEXT("%s?listen"), *MapPathSubsystem->GetLobbyMapPath());
                NumPublicConnections = NumberOfPublicConnections;
                SessionName = FName(InSessionName);
            }
        }
    }

    // -------------------------------------------------------------------------
    // Bind Delegates:
    // Bind our callback functions to the subsystem's delegates.
    // To prevent multiple calls, ensure these bindings occur only once per widget instance.
    // -------------------------------------------------------------------------
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
        MultiplayerSessionsSubsystem->MultiplayerOnSessionFailure.AddUObject(this, &ThisClass::OnSessionFailure);
    }
}

// -----------------------------------------------------------------------------
// Initialize:
// Called when the widget is first constructed; binds button click events.
// To prevent callbacks from being bound multiple times, these should only be bound once per widget instance.
// -----------------------------------------------------------------------------
bool UCommonMultiplayerLobbyCreation::Initialize()
{
    if (!Super::Initialize())
    {
        return false;
    }

    // Bind the Host button click event.
    if (CreateLobbyButton)
    {
        CreateLobbyButton->OnClicked().AddUObject(this, &ThisClass::CreateLobbyButtonClicked);
    }
    return true;
}

// -----------------------------------------------------------------------------
// NativeDestruct:
// Called when the widget is about to be destroyed.
// Unbind delegates here to prevent multiple callback invocations after destruction.
// -----------------------------------------------------------------------------
void UCommonMultiplayerLobbyCreation::NativeDestruct()
{
    // Unbind all delegates from the subsystem to avoid duplicate calls.
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.RemoveAll(this);
        MultiplayerSessionsSubsystem->MultiplayerOnSessionFailure.RemoveAll(this);
    }

    Super::NativeDestruct();
}

// -----------------------------------------------------------------------------
// OnCreateSession:
// Callback for when a session creation attempt is complete.
// If successful, retrieves the session's connection string and travels to the lobby map.
// -----------------------------------------------------------------------------
void UCommonMultiplayerLobbyCreation::OnCreateSession(bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        // Retrieve the online subsystem to access session info.
        IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
        if (Subsystem)
        {
            IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
            if (SessionInterface.IsValid())
            {
                // Get the resolved connection string for debugging and joining.
                FString Address;
                SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(
                        -1,
                        5.f,
                        FColor::Green,
                        FString::Printf(TEXT("On Create Session -> Address: %s "), *Address)
                        );
                }
            }
        }

        // Notify Blueprints that session creation succeeded.
        IsCreateSessionSuccessful(bWasSuccessful);

        // Only the host (listen-server) will have authority GameMode
        if (UWorld* World = GetWorld())
        {
            if (AMultiplayerMenuGameMode* GM = World->GetAuthGameMode<AMultiplayerMenuGameMode>())
            {
                // Delay a tick to ensure all session RPCs are processed
                World->GetTimerManager().SetTimerForNextTick([GM, this]() {
                    GM->TravelToLobby(PathToLobby);
                });
            }
            else
            {
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
                        FString::Printf(TEXT("Client UI attempted to travel; only server can ServerTravel.")));
                }
            }
        }
    }
    else
    {
        // If session creation failed, re-enable the Host button for another attempt.
        CreateLobbyButton->SetIsEnabled(true);
        CancelButton->SetIsEnabled(true);
    }
}

// -----------------------------------------------------------------------------
// OnSessionFailure:
// Callback to handle session failures (such as lost connection).
// Logs the UniqueNetId and failure type for debugging.
// -----------------------------------------------------------------------------
void UCommonMultiplayerLobbyCreation::OnSessionFailure(const FUniqueNetId& UniqueNetId, ESessionFailure::Type SessionFailureType)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
            FString::Printf(TEXT("UCommonMultiplayerLobbyCreation::OnSessionFailure for player: %s, Failure type: %s"),
                *UniqueNetId.ToString(), LexToString(SessionFailureType)));
    }

    CreateLobbyButton->SetIsEnabled(true);
}

void UCommonMultiplayerLobbyCreation::CreateLobbyButtonClicked()
{
    CreateLobbyButton->SetIsEnabled(false);
    CancelButton->SetIsEnabled(false);
}

void UCommonMultiplayerLobbyCreation::CancelButtonClicked()
{
    CreateLobbyButton->SetIsEnabled(false);
}