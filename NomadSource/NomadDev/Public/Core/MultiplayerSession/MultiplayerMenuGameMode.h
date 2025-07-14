// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MultiplayerMenuGameMode.generated.h"

/**
 * 
 */
UCLASS()
class NOMADDEV_API AMultiplayerMenuGameMode : public AGameMode
{
	GENERATED_BODY()

public:

    AMultiplayerMenuGameMode();

    /**
     * Initiates server travel to the lobby map.
     */
    void TravelToLobby(const FString& PathToLobby);
};
