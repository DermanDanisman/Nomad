// -----------------------------------------------------------------------------
// UCommonMultiplayerLobbyMenu.h
//
// This widget manages the lobby menu for multiplayer sessions. It handles
// updating the player list, toggling ready states, starting the session, and
// closing the lobby. It uses the Common UI framework and communicates with
// the MultiplayerSessionsSubsystem to perform session operations.
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Interfaces/OnlineSessionInterface.h"         // Online session functionality
#include "CommonMultiplayerLobbyMenu.generated.h"

// Forward declarations for classes and structures used by the lobby UI.
class ALobbyPlayerController;
class UMultiplayerSessionsSubsystem;
class UButton;
class UCommonButtonBase;
struct FPlayerInfo;

/**
 *
 */
UCLASS()
class NOMADDEV_API UCommonMultiplayerLobbyMenu : public UCommonActivatableWidget {
    GENERATED_BODY()

public:
    // -------------------------------------------------------------------------
    // NativeOnActivated:
    // Called when the widget is activated. Use this to reset or enable UI elements.
    // -------------------------------------------------------------------------
    virtual void NativeOnActivated() override;

    /**
     * MenuSetup:
     * Initializes the lobby menu with the specified game map path and lobby mode.
     * - GamePath: The map to travel to when the session starts.
     * - bIsInLobby: True if the UI is in a lobby state (affects button visibility).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | LobbyMenu")
    void MenuSetup(bool bIsInLobby = true);

    /**
     * Blueprint-implementable event to update the displayed player list.
     * The array of FPlayerInfo is passed to Blueprints for UI update.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Multiplayer Sessions Subsystem | LobbyMenu")
    void UpdatePlayerList(const TArray<FPlayerInfo>& PlayerInfo);

    /**
     * Returns whether the local player is marked as ready.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | LobbyMenu")
    bool GetIsPlayerReadyStatus();

protected:
    // -------------------------------------------------------------------------
    // Widget Lifecycle:
    // Initialize is called once when the widget is created.
    // NativeDestruct is called before the widget is destroyed.
    // -------------------------------------------------------------------------
    virtual bool Initialize() override;
    virtual void NativeDestruct() override;

    // -------------------------------------------------------------------------
    // Session Callbacks:
    // These functions are bound to the session subsystem delegates.
    // They are responsible for handling session start, destruction, and end events.
    // -------------------------------------------------------------------------
    UFUNCTION()
    void OnStartSession(bool bWasSuccessful);

    UFUNCTION()
    void OnDestroySession(bool bWasSuccessful);

    UFUNCTION()
    void OnEndSession(bool bWasSuccessful);

    /**
     * Callback triggered when the start session action completes.
     * This may be used to enable/disable the Start button.
     */
    UFUNCTION()
    void OnStartSessionActionCompleted(bool bWasSuccessful);

    /**
     * Called when a session failure occurs.
     */
    void OnSessionFailure(const FUniqueNetId& UniqueNetId, ESessionFailure::Type SessionFailureType);

private:
    // -------------------------------------------------------------------------
    // UI Buttons:
    // These are bound via the widget blueprint.
    // -------------------------------------------------------------------------
    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Sessions Subsystem | LobbyMenu | UI", meta = (BindWidget), meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCommonButtonBase> CloseLobbyButton;

    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Sessions Subsystem | LobbyMenu | UI", meta = (BindWidget), meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCommonButtonBase> ReadyButton;

    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Sessions Subsystem | LobbyMenu | UI", meta = (BindWidget), meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCommonButtonBase> StartButton;

    // -------------------------------------------------------------------------
    // Button Callback Functions:
    // These functions are called when the user clicks the corresponding button.
    // Consider disabling the button immediately or using a flag to prevent multiple calls.
    // -------------------------------------------------------------------------
    UFUNCTION()
    void CloseLobbyButtonClicked();

    UFUNCTION()
    void ReadyButtonClicked();

    UFUNCTION()
    void StartButtonClicked();

    /**
     * MenuTearDown:
     * Cleans up the menu by removing it from the viewport and resetting input mode.
     */
    void MenuTearDown();

    // -------------------------------------------------------------------------
    // Session Subsystem Reference:
    // Pointer to the subsystem that handles session creation, joining, destruction, etc.
    // -------------------------------------------------------------------------
    TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

    // -------------------------------------------------------------------------
    // Cached References:
    // Pointers to the owning player controller and the world.
    // Using GetFirstLocalPlayerController is recommended for UI ownership.
    // -------------------------------------------------------------------------
    UPROPERTY()
    TObjectPtr<APlayerController> PlayerController;

    UPROPERTY()
    TObjectPtr<UWorld> CurrentWorld;

    // -------------------------------------------------------------------------
    // Session Settings:
    // PathToGame is the map path (with ?listen appended) used when starting the session.
    // -------------------------------------------------------------------------
    FString PathToGame{ TEXT("") };
};