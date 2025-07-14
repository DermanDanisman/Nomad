// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/MultiplayerSession/PlayerController/MenuPlayerController.h"

// -----------------------------------------------------------------------------
// MCWI_PushMenu_Implementation:
// Forwards the call to the interface's default implementation.
// -----------------------------------------------------------------------------
void AMenuPlayerController::MCWI_PushMenu_Implementation(TSubclassOf<UCommonActivatableWidget> ActivatableWidget)
{
    IMenuControllerWidgetInterface::MCWI_PushMenu_Implementation(ActivatableWidget);
}

// -----------------------------------------------------------------------------
// MCWI_PushHUD_Implementation:
// Forwards the call to push a HUD widget to the interface's default behavior.
// -----------------------------------------------------------------------------
void AMenuPlayerController::MCWI_PushHUD_Implementation(TSubclassOf<UCommonActivatableWidget> ActivatableWidget)
{
    IMenuControllerWidgetInterface::MCWI_PushHUD_Implementation(ActivatableWidget);
}

// -----------------------------------------------------------------------------
// MCWI_PushMenuPrompt_Implementation:
// Forwards the prompt request to the default implementation.
// If duplicate callbacks become an issue, you might consider adding a check or
// debounce flag here to prevent multiple prompts from being pushed.
// -----------------------------------------------------------------------------
void AMenuPlayerController::MCWI_PushMenuPrompt_Implementation(TSubclassOf<UCommonActivatableWidget> ActivatableWidget, EPromptIndex PromptIndex, EPromptToStack PromptToStack, const FText& PromptText)
{
    IMenuControllerWidgetInterface::MCWI_PushMenuPrompt_Implementation(ActivatableWidget, PromptIndex, PromptToStack, PromptText);
}