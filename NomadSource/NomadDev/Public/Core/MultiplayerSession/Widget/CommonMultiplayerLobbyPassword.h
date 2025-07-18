// -----------------------------------------------------------------------------
// UCommonMultiplayerLobbyPassword.h
//
// This widget represents the UI used to join a lobby session when a password
// is required. It interacts with the MultiplayerSessionsSubsystem to join a
// session, and displays error messages (via a Blueprint event) if the provided
// password is incorrect.
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"                         // Base class for common UI widgets.
#include "CommonButtonBase.h"                         // For Common UI button types.
#include "Interfaces/OnlineSessionInterface.h"        // For online session functionality.
#include "CommonMultiplayerLobbyPassword.generated.h"

// Forward declarations
class UMultiplayerSessionsSubsystem;
class UButton;
class UCommonButtonBase;

UCLASS()
class NOMADDEV_API UCommonMultiplayerLobbyPassword : public UCommonUserWidget {
    GENERATED_BODY()

public:
    /**
     * MenuSetup:
     * Initializes the lobby password widget. Retrieves the session subsystem,
     * binds necessary callbacks for join session completion and password checking.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | Lobby Password")
    void MenuSetup();

    /**
     * CallWrongPasswordPopup:
     * Blueprint event used to display a wrong-password message on the UI.
     * The Blueprint should implement this event to show a popup or other feedback.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Multiplayer Sessions Subsystem | Lobby Password")
    void CallWrongPasswordPopup(bool PasswordMatched);

    /**
     * IsJoinSessionSuccessful:
     * Blueprint event to notify Blueprints whether the join session operation was successful.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Multiplayer Sessions Subsystem | Lobby Password")
    void IsJoinSessionSuccessful(bool bIsSuccessful);

protected:
    // -------------------------------------------------------------------------
    // Widget Lifecycle:
    // Initialize is called once when the widget is created; NativeDestruct is called
    // before the widget is destroyed.
    // -------------------------------------------------------------------------
    virtual bool Initialize() override;
    virtual void NativeDestruct() override;

    /**
     * OnJoinSession:
     * Callback invoked when a join session attempt completes.
     * Processes the join result, retrieves the connection string, and travels to the session.
     */
    void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);

    /**
     * OnSessionFailure:
     * Callback for handling session failures. Logs details about the failure.
     */
    void OnSessionFailure(const FUniqueNetId& UniqueNetId, ESessionFailure::Type SessionFailureType);

private:
    // -------------------------------------------------------------------------
    // UI Widgets:
    // These pointers are bound to UI elements in the widget blueprint.
    // -------------------------------------------------------------------------
    UPROPERTY(meta = (BindWidget), Category = "Multiplayer Sessions Subsystem | Lobby Password | UI", BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> JoinButton;

    // -------------------------------------------------------------------------
    // Button Callback Functions:
    // Called when the Join button is pressed. Disabling the button immediately
    // helps prevent duplicate invocations.
    // -------------------------------------------------------------------------
    UFUNCTION()
    void JoinButtonClicked();

    // -------------------------------------------------------------------------
    // Session Subsystem Reference:
    // Pointer to the custom MultiplayerSessionsSubsystem for session operations.
    // -------------------------------------------------------------------------
    TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;
};