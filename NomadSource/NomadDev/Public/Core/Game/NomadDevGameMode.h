// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/ACFGameMode.h"
#include "NomadDevGameMode.generated.h"

class ANomadPlayerController;

UCLASS(minimalapi)
class ANomadDevGameMode : public AACFGameMode
{
    GENERATED_BODY()

public:
    ANomadDevGameMode();

    virtual void GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList) override;

    // Called when a new player logs in.
    virtual void PostLogin(APlayerController* NewPlayer) override;

    // Called when a player logs out.
    virtual void Logout(AController* Exiting) override;

    virtual void StartPlay() override;

    virtual void GameWelcomePlayer(UNetConnection* Connection, FString& RedirectURL) override;

    virtual void HandleSeamlessTravelPlayer(AController*& C) override;

    TArray<TObjectPtr<ANomadPlayerController>> PlayerController;

    FTimerHandle DelayedCustomizationHandle;

};