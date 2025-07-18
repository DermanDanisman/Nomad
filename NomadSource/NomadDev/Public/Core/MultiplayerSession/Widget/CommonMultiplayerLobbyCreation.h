// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "CommonMultiplayerLobbyCreation.generated.h"

// Forward declarations
class UMultiplayerSessionsSubsystem;
class UCommonButtonBase;

/**
 *
 */
UCLASS()
class NOMADDEV_API UCommonMultiplayerLobbyCreation : public UCommonActivatableWidget {
    GENERATED_BODY()

public:
    // -------------------------------------------------------------------------
    // NativeOnActivated:
    // Called when the widget becomes active.
    // Use this to enable buttons or reset UI state.
    // -------------------------------------------------------------------------
    virtual void NativeOnActivated() override;

    /**
     * MenuSetup:
     * Configures the menu by setting session parameters, initializing the session subsystem,
     * binding delegates, and adjusting button visibility based on lobby state.
     *
     * @param NumberOfPublicConnections - Maximum number of players.
     * @param InSessionName - The name used for session display and filtering.
     * @param LobbyPath - Map path for the lobby (appended with ?listen).
     * @param bIsInLobby - If true, configures the UI for an in-lobby state.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | Lobby Creation")
    void MenuSetup(int32 NumberOfPublicConnections = 4, FString InSessionName = FString(TEXT("ThisSession")), bool bIsInLobby = false);

    /**
     * Blueprint event to notify whether session creation was successful.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Multiplayer Sessions Subsystem | Lobby Creation")
    void IsCreateSessionSuccessful(bool bIsSuccessful);

protected:
    // -------------------------------------------------------------------------
    // Widget Lifecycle
    // -------------------------------------------------------------------------
    // Initialize:
    // Called when the widget is first constructed. Bind button events here.
    virtual bool Initialize() override;
    // NativeDestruct:
    // Called when the widget is about to be destroyed. Unbind delegates and perform cleanup.
    virtual void NativeDestruct() override;

    // -------------------------------------------------------------------------
    // Session Subsystem Delegate Callbacks:
    // These functions are called by the subsystem when session operations complete.
    // -------------------------------------------------------------------------
    // OnCreateSession:
    // Called when a session creation attempt completes.
    UFUNCTION()
    void OnCreateSession(bool bWasSuccessful);

    /**
     * OnSessionFailure:
     * Callback for handling session failures. Logs details about the failure.
     */
    void OnSessionFailure(const FUniqueNetId& UniqueNetId, ESessionFailure::Type SessionFailureType);

private:
    // -------------------------------------------------------------------------
    // UI Widgets:
    // Bound to UI elements in the widget blueprint.
    // -------------------------------------------------------------------------

    UPROPERTY(meta = (BindWidget), Category = "Multiplayer Sessions Subsystem | Lobby Creation | UI", BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> CreateLobbyButton;

    UPROPERTY(meta = (BindWidget), Category = "Multiplayer Sessions Subsystem | Lobby Creation | UI", BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> CancelButton;

    // -------------------------------------------------------------------------
    // Button Callback Functions:
    // Called when the respective buttons are pressed.
    // -------------------------------------------------------------------------

    UFUNCTION()
    void CreateLobbyButtonClicked();

    UFUNCTION()
    void CancelButtonClicked();

    // -------------------------------------------------------------------------
    // Session Subsystem Reference:
    // Pointer to the custom session subsystem used for session operations.
    // -------------------------------------------------------------------------
    TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

    // -------------------------------------------------------------------------
    // Session Settings:
    // Local variables to store session configuration.
    // -------------------------------------------------------------------------
    int32 NumPublicConnections{ 4 };
    FName SessionName{ TEXT("ThisSession") };
    FString PathToLobby{ TEXT("") };
};