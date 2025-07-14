// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/MultiplayerSession/MultiplayerMenuGameMode.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"

AMultiplayerMenuGameMode::AMultiplayerMenuGameMode()
{
    bUseSeamlessTravel = false;
}

void AMultiplayerMenuGameMode::TravelToLobby(const FString& PathToLobby)
{
    // Relative travel preserves "?listen"
    const FString LobbyURL = PathToLobby + TEXT("?listen"); // e.g. "/Game/Maps/MyLobby?listen"
    GetWorld()->ServerTravel(LobbyURL, /*bAbsolute=*/false);
}