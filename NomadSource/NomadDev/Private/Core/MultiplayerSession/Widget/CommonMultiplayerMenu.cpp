// -----------------------------------------------------------------------------
// UCommonMultiplayerMenu.cpp
// Implementation of UCommonMultiplayerMenu functionality for session management.
// -----------------------------------------------------------------------------

#include "Core/MultiplayerSession/Widget/CommonMultiplayerMenu.h"
#include "Subsystem/MultiplayerSessionsSubsystem.h"
#include "Components/Button.h"
#include "Core/MultiplayerSession/PlayerController/MenuPlayerController.h" // For custom PlayerController if needed
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

// -----------------------------------------------------------------------------
// NativeOnActivated:
// Called when the widget becomes active. We enable main menu buttons here.
// -----------------------------------------------------------------------------
void UCommonMultiplayerMenu::NativeOnActivated()
{
    Super::NativeOnActivated();
    // Ensure buttons are enabled when the menu is activated.
}

// -----------------------------------------------------------------------------
// MenuSetup:
// Configures the session parameters, retrieves the session subsystem, binds delegates,
// and sets button visibility based on whether we are in the lobby.
// -----------------------------------------------------------------------------
void UCommonMultiplayerMenu::MenuSetup()
{
    // -------------------------------------------------------------------------
    // Retrieve the MultiplayerSessionsSubsystem from the GameInstance.
    // -------------------------------------------------------------------------
    const UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    }

    // -------------------------------------------------------------------------
    // Bind Delegates:
    // Bind our callback functions to the subsystem's delegates.
    // To prevent multiple calls, ensure these bindings occur only once per widget instance.
    // -------------------------------------------------------------------------
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnSessionFailure.AddUObject(this, &ThisClass::OnSessionFailure);
    }
}

// -----------------------------------------------------------------------------
// Initialize:
// Called when the widget is first constructed; binds button click events.
// To prevent callbacks from being bound multiple times, these should only be bound once per widget instance.
// -----------------------------------------------------------------------------
bool UCommonMultiplayerMenu::Initialize()
{
    if (!Super::Initialize())
    {
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
// NativeDestruct:
// Called when the widget is about to be destroyed.
// Unbind delegates here to prevent multiple callback invocations after destruction.
// -----------------------------------------------------------------------------
void UCommonMultiplayerMenu::NativeDestruct()
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
// OnSessionFailure:
// Callback to handle session failures (such as lost connection).
// Logs the UniqueNetId and failure type for debugging.
// -----------------------------------------------------------------------------
void UCommonMultiplayerMenu::OnSessionFailure(const FUniqueNetId& UniqueNetId, ESessionFailure::Type SessionFailureType)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
            FString::Printf(TEXT("Session failure for player: %s, Failure type: %s"),
                *UniqueNetId.ToString(), LexToString(SessionFailureType)));
    }
}

// -----------------------------------------------------------------------------
// Interface Functions for Prompt Handling:
// These are the implementations of the IWidgetPromptInterface functions.
// Currently, they just call the default behavior, but custom logic can be added if necessary.
// -----------------------------------------------------------------------------
void UCommonMultiplayerMenu::WPI_PromptConfirmed_Implementation(EPromptIndex PromptIndex)
{
    IWidgetPromptInterface::WPI_PromptConfirmed_Implementation(PromptIndex);
}

void UCommonMultiplayerMenu::WPI_PromptCanceled_Implementation(EPromptIndex PromptIndex)
{
    IWidgetPromptInterface::WPI_PromptCanceled_Implementation(PromptIndex);
}