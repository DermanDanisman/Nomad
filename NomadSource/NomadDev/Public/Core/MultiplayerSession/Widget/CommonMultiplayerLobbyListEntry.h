// -----------------------------------------------------------------------------
// UCommonMultiplayerLobbyListEntry.h
//
// This widget represents a single entry in the lobby list. Each entry displays 
// information about a multiplayer session and allows the player to join that session.
// It uses the Online Session Interface to retrieve connection details and triggers 
// a session join when the Join button is clicked.
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"            // For UI button types.
#include "CommonUserWidget.h"            // Base class for user widgets.
#include "Interfaces/OnlineSessionInterface.h"   // For online session functionality.
#include "CommonMultiplayerLobbyListEntry.generated.h"

// Forward Declaration of classes used in this widget.
class UMultiplayerSessionsSubsystem;
class UButton;
class UCommonButtonBase;

UCLASS()
class NOMADDEV_API UCommonMultiplayerLobbyListEntry : public UCommonUserWidget {
    GENERATED_BODY()

public:
    /**
     * MenuSetup:
     * Initializes this lobby list entry. It retrieves the MultiplayerSessionsSubsystem 
     * and binds the join session callback. Optionally, you might perform additional 
     * setup here.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | Lobby List Entry")
    void MenuSetup();

    /**
     * Blueprint event to notify Blueprints whether joining the session was successful.
     * This event can be implemented in Blueprints to update the UI accordingly.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Multiplayer Sessions Subsystem | Lobby List Entry")
    void IsJoinSessionSuccessful(bool bIsSuccessful);

    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | Lobby List Entry")
    bool SetHasSessionPassword(bool bInHasSessionPassword)
    {
        return bHasSessionPassword = bInHasSessionPassword;
    };

    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | Lobby List Entry")
    bool GetHasSessionPassword() const
    {
        return bHasSessionPassword;
    };

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
     * Callback that is triggered when a join session attempt completes.
     * It retrieves the session connection string and initiates client travel.
     */
    void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);

    /**
     * Called when a session failure occurs.
     */
    void OnSessionFailure(const FUniqueNetId& UniqueNetId, ESessionFailure::Type SessionFailureType);

private:
    // -------------------------------------------------------------------------
    // UI Widgets:
    // These pointers are bound to the UI elements in the corresponding widget blueprint.
    // -------------------------------------------------------------------------
    UPROPERTY(meta = (BindWidget), Category = "Multiplayer Sessions Subsystem | Lobby List Entry | UI", BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> JoinButton;

    // -------------------------------------------------------------------------
    // Button Callback Functions:
    // Called when the Join button is clicked.
    // -------------------------------------------------------------------------
    UFUNCTION()
    void JoinButtonClicked();

    // -------------------------------------------------------------------------
    // Session Subsystem Reference:
    // A pointer to the MultiplayerSessionsSubsystem that handles session operations.
    // -------------------------------------------------------------------------
    TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

    bool bHasSessionPassword = false;
};