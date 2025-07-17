// -----------------------------------------------------------------------------
// UCommonMultiplayerMenu.h
// This widget manages the main multiplayer menu using the Common UI system.
// It allows the user to host or find sessions, and handles UI prompts.
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Interfaces/OnlineSessionInterface.h"   // Online session functionality
#include "Interface/WidgetPromptInterface.h"       // For prompt confirmation/cancellation
#include "CommonMultiplayerMenu.generated.h"

// Forward declarations
class UMultiplayerSessionsSubsystem;
class UCommonButtonBase;

/**
 * UCommonMultiplayerMenu:
 * - Inherits from UCommonActivatableWidget for common UI activation behavior.
 * - Implements IWidgetPromptInterface for handling user prompt confirmations/cancellations.
 */
UCLASS()
class NOMADDEV_API UCommonMultiplayerMenu : public UCommonActivatableWidget, public IWidgetPromptInterface {
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
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions Subsystem | Menu")
    void MenuSetup();

    /**
     * OnSessionFailure:
     * Callback for handling session failures. Logs details about the failure.
     */
    void OnSessionFailure(const FUniqueNetId& UniqueNetId, ESessionFailure::Type SessionFailureType);

    // -------------------------------------------------------------------------
    // Interface Functions (for prompt confirmation/cancellation)
    // These are implemented as part of IWidgetPromptInterface.
    // -------------------------------------------------------------------------
    virtual void WPI_PromptConfirmed_Implementation(EPromptIndex PromptIndex) override;
    virtual void WPI_PromptCanceled_Implementation(EPromptIndex PromptIndex) override;

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

private:
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