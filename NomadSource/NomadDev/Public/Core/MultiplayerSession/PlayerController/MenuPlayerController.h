// -----------------------------------------------------------------------------
// MenuPlayerController.h
//
// This PlayerController implements the IMenuControllerWidgetInterface, which
// defines functions for pushing various UI widgets (menus, HUDs, prompts) onto
// the screen. This implementation simply forwards the calls to the default
// implementations provided by the interface. If custom behavior is needed in
// the future, you can modify these functions.
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interface/MenuControllerWidgetInterface.h"
#include "MenuPlayerController.generated.h"

// Forward declarations for UI widget classes.
class UCommonActivatableWidget;
class UUserWidget;

UCLASS()
class NOMADDEV_API AMenuPlayerController : public APlayerController, public IMenuControllerWidgetInterface {
    GENERATED_BODY()

public:
    // -------------------------------------------------------------------------
    // MCWI_PushMenu_Implementation:
    // Called when a menu widget should be pushed onto the UI stack.
    // Currently, it forwards the call to the default interface implementation.
    // -------------------------------------------------------------------------
    virtual void MCWI_PushMenu_Implementation(TSubclassOf<UCommonActivatableWidget> ActivatableWidget) override;

    // -------------------------------------------------------------------------
    // MCWI_PushHUD_Implementation:
    // Called when a HUD widget should be pushed onto the UI stack.
    // This simply forwards the call to the default implementation.
    // -------------------------------------------------------------------------
    virtual void MCWI_PushHUD_Implementation(TSubclassOf<UCommonActivatableWidget> ActivatableWidget) override;

    // -------------------------------------------------------------------------
    // MCWI_PushMenuPrompt_Implementation:
    // Called to push a prompt (e.g., "Are you sure you want to exit?") onto the UI.
    // Accepts a prompt index and stack identifier along with the widget class and text.
    // Forwards the call to the default implementation.
    // -------------------------------------------------------------------------
    virtual void MCWI_PushMenuPrompt_Implementation(TSubclassOf<UCommonActivatableWidget> ActivatableWidget, EPromptIndex PromptIndex, EPromptToStack PromptToStack, const FText& PromptText) override;
};