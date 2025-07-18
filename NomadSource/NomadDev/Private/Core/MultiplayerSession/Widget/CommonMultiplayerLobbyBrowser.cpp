// -----------------------------------------------------------------------------
// UCommonMultiplayerLobbyBrowser.cpp
//
// Implements the lobby browser widget functionality. It sets up the lobby
// search by obtaining the session subsystem, binding delegates, and triggering
// the session search. Results are processed and passed to Blueprints.
// -----------------------------------------------------------------------------

#include "Core/MultiplayerSession/Widget/CommonMultiplayerLobbyBrowser.h"

#include "CommonButtonBase.h"
#include "Subsystem/MultiplayerSessionsSubsystem.h"     // To call session functions
#include "Components/Button.h"							// For UButton bindings
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/GameInstance.h"
#include "OnlineSubsystemTypes.h"
#include "Engine/Engine.h"

void UCommonMultiplayerLobbyBrowser::MenuSetup()
{
    // -------------------------------------------------------------------------
    // Get Session Subsystem:
    // Retrieve the MultiplayerSessionsSubsystem from the GameInstance.
    // This subsystem handles the session search, join, etc.
    // -------------------------------------------------------------------------
    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    }

    // -------------------------------------------------------------------------
    // Bind Subsystem Delegates:
    // Bind the OnFindSessions delegate to our OnFindSessions function.
    // This ensures that once the search completes, our callback is triggered.
    // -------------------------------------------------------------------------
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
    }

    // -------------------------------------------------------------------------
    // Trigger Session Search:
    // Start a new session search immediately.
    // It may be a good idea to disable the SearchLobbiesButton here to avoid
    // duplicate searches.
    // -------------------------------------------------------------------------
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->FindSessions(10000);
    }
}

void UCommonMultiplayerLobbyBrowser::NativeOnActivated()
{
    Super::NativeOnActivated();
    // You could add logic here to reset UI elements when the widget becomes active.
}

bool UCommonMultiplayerLobbyBrowser::Initialize()
{
    if (!Super::Initialize())
    {
        return false;
    }

    // Bind the Search Lobbies button click event to its callback.
    // To prevent multiple bindings, ensure this is only called once.
    if (SearchLobbiesButton)
    {
        SearchLobbiesButton->OnClicked().AddUObject(this, &ThisClass::SearchLobbiesButtonClicked);
    }
    return true;
}

void UCommonMultiplayerLobbyBrowser::NativeDestruct()
{
    // Unbind the delegate to avoid any callbacks after this widget is destroyed.
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.RemoveAll(this);
    }

    Super::NativeDestruct();
}

void UCommonMultiplayerLobbyBrowser::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
    // If the subsystem is not valid, return early.
    if (MultiplayerSessionsSubsystem == nullptr)
    {
        return;
    }

    // Debug message for tracking when this callback is triggered.
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            5.f,
            FColor::Cyan,
            TEXT("UCommonMultiplayerLobbyBrowser::OnFindSessions")
            );
    }

    // Clear any previous session results.
    BlueprintSessionResults.Empty();

    // Get the local player controller to help with session filtering.
    APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();

    // Process each session search result.
    for (const FOnlineSessionSearchResult& Result : SessionResults)
    {
        if (!Result.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid session found, skipping..."));
            continue;
        }

        // Convert to a Blueprint-friendly format.
        FBlueprintSessionResult SessionResult;
        SessionResult.OnlineResult = Result;
        BlueprintSessionResults.Add(SessionResult);

        // Ensure lobbies are used if available.
        SessionResult.OnlineResult.Session.SessionSettings.bUseLobbiesIfAvailable = true;

        // Example filtering: if the session belongs to the local player, optionally destroy it.
        FUniqueNetIdRepl LocalPlayerId = PlayerController->GetLocalPlayer()->GetUniqueNetIdForPlatformUser();
        if (LocalPlayerId.IsValid() && Result.Session.OwningUserId.IsValid())
        {
            if (Result.Session.OwningUserId->ToString().Equals(LocalPlayerId->ToString()))
            {
                MultiplayerSessionsSubsystem->DestroySession();
            }
        }
    }

    // If any valid sessions were found, pass the results to Blueprints.
    if (BlueprintSessionResults.Num() > 0)
    {
        FindSessionResultCompleted(BlueprintSessionResults);
    }

    // Debug message: display how many sessions were found.
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            5.f,
            FColor::Cyan,
            FString::Printf(TEXT("Found %d sessions"), SessionResults.Num())
            );
    }

    SearchLobbiesButton->SetIsEnabled(true);
}

void UCommonMultiplayerLobbyBrowser::OnFindSessionSearchState(EOnlineAsyncTaskState::Type FindSessionSearchState)
{
    // Enable the Search button when the search is complete or failed,
    // and disable it while the search is in progress.
    if (FindSessionSearchState == EOnlineAsyncTaskState::Done ||
        FindSessionSearchState == EOnlineAsyncTaskState::NotStarted ||
        FindSessionSearchState == EOnlineAsyncTaskState::Failed)
    {
        SearchLobbiesButton->SetIsEnabled(true);
    } else
    {
        SearchLobbiesButton->SetIsEnabled(false);
    }
}

void UCommonMultiplayerLobbyBrowser::SearchLobbiesButtonClicked()
{
    // Immediately disable the button to prevent duplicate clicks.

    // Trigger a new session search via the subsystem.
    if (MultiplayerSessionsSubsystem)
    {
        SearchLobbiesButton->SetIsEnabled(false);
        MultiplayerSessionsSubsystem->FindSessions(10000);
    }

}