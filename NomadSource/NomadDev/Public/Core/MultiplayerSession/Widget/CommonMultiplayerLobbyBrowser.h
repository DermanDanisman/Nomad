// -----------------------------------------------------------------------------
// UCommonMultiplayerLobbyBrowser.h
//
// This widget displays a lobby browser UI that allows players to search for
// available multiplayer sessions. It uses the MultiplayerSessionsSubsystem to
// perform the session search and then passes the results to Blueprints via an
// event (FindSessionResultCompleted).
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "OnlineSessionSettings.h"               // For session settings
#include "Interfaces/OnlineSessionInterface.h"   // For online session functionality
#include "FindSessionsCallbackProxy.h"           // For FBlueprintSessionResult and session search results
#include "CommonMultiplayerLobbyBrowser.generated.h"

// Forward Declaration
class UMultiplayerSessionsSubsystem;
class UButton;
class UCommonButtonBase;
/**
 *
 */
UCLASS()
class NOMADDEV_API UCommonMultiplayerLobbyBrowser : public UCommonActivatableWidget {
    GENERATED_BODY()

public:
    /**
     * MenuSetup:
     * Initializes the lobby browser. Retrieves the session subsystem,
     * binds the session search delegate, and starts the session search.
     * This method should be called once when opening the lobby browser.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | Lobby Browser")
    void MenuSetup();

    /**
     * NativeOnActivated:
     * Called when the widget is activated. Can be used to reset UI state.
     */
    virtual void NativeOnActivated() override;

protected:
    // -------------------------------------------------------------------------
    // Widget Lifecycle
    // -------------------------------------------------------------------------
    // Initialize:
    // Called once when the widget is constructed. Bind button click events here.
    virtual bool Initialize() override;
    // NativeDestruct:
    // Called when the widget is about to be destroyed. Unbind delegates to avoid
    // callbacks to a destroyed widget.
    virtual void NativeDestruct() override;

    // -------------------------------------------------------------------------
    // Session Subsystem Delegate Callbacks
    // -------------------------------------------------------------------------
    /**
     * OnFindSessions:
     * Called when the session search is complete.
     * Processes the results and calls the Blueprint event to update the UI.
     */
    void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);

    /**
     * OnFindSessionSearchState:
     * Called whenever the search state changes.
     * Used here to enable or disable the Search button based on the state.
     */
    void OnFindSessionSearchState(EOnlineAsyncTaskState::Type FindSessionSearchState);

    // -------------------------------------------------------------------------
    // Blueprint-Friendly Session Results
    // -------------------------------------------------------------------------
    // Holds the session search results that will be sent to Blueprints.
    UPROPERTY()
    TArray<FBlueprintSessionResult> BlueprintSessionResults;

    /**
     * FindSessionResultCompleted:
     * Blueprint event used to pass the search results to the UI.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Multiplayer Sessions Subsystem | Lobby Browser")
    void FindSessionResultCompleted(const TArray<FBlueprintSessionResult>& SessionInfos);

private:
    // -------------------------------------------------------------------------
    // UI Widgets
    // -------------------------------------------------------------------------
    // Button for triggering a search for lobbies.
    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Sessions Subsystem | Lobby Browser | UI", meta = (BindWidget), meta = (AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> SearchLobbiesButton;

    // -------------------------------------------------------------------------
    // Button Callback Functions
    // -------------------------------------------------------------------------
    /**
     * SearchLobbiesButtonClicked:
     * Called when the Search Lobbies button is pressed.
     * It triggers the session search in the subsystem.
     * To prevent duplicate callbacks, the button should be disabled immediately.
     */
    UFUNCTION()
    void SearchLobbiesButtonClicked();

    // -------------------------------------------------------------------------
    // Session Subsystem Reference
    // -------------------------------------------------------------------------
    // Pointer to the custom subsystem that handles session operations.
    TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;
};