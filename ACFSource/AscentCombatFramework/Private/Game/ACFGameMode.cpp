// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#include "Game/ACFGameMode.h"
#include "ACMCollisionsMasterComponent.h"
#include "Components/ACFRagdollMasterComponent.h"
#include "Game/ACFPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameSession.h"

#include <Kismet/GameplayStatics.h>


AACFGameMode::AACFGameMode()
{
    bUseSeamlessTravel = true;
    CollisionManager = CreateDefaultSubobject<UACMCollisionsMasterComponent>(TEXT("Collision Master Comp"));
    RagdollManager = CreateDefaultSubobject<UACFRagdollMasterComponent>(TEXT("Ragdoll Master Comp"));
}

void AACFGameMode::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
    Super::GetSeamlessTravelActorList(bToTransition, ActorList);

    /*if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
            FString::Printf(TEXT("ACF Game Mode --> GetSeamlessTravelActorList")));
    }

    // Get allocations for the elements we're going to add handled in one go
    const int32 ActorsToAddCount = GameState->PlayerArray.Num() + (bToTransition ? 3 : 0);
    ActorList.Reserve(ActorsToAddCount);

    if (bToTransition) // true if we are going from old level to transition map, false if we are going from transition map to new level
    {
        // Always keep PlayerStates, so that after we restart we can keep players on the same team, etc
        ActorList.Append(GameState->PlayerArray);
        // Keep ourselves until we transition to the transition map
        ActorList.Add(this);
        // Keep general game state until we transition to the transition map
        ActorList.Add(GameState);
        // Keep the game session state until we transition to the transition map
        ActorList.Add(GameSession);

        // If adding in this section best to increase the literal above for the ActorsToAddCount
    }*/
}

// void AACFGameMode::SpawnPlayersCompanions()
// {
// 	TArray<class AACFPlayerController*> pcs = GetAllPlayerControllers();
// 
// 	for (AACFPlayerController* pc : pcs) {
// 		pc->GetCompanionsComponent()->SpawnGroup();
// 	}
// }

TArray<AACFPlayerController*> AACFGameMode::GetAllPlayerControllers()
{
    TArray<AACFPlayerController*> PlayerControllers;
    const int32 NumOfPlayers = GetNumPlayers();

    for (int32 i = 0; i < NumOfPlayers; i++)
    {
        APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, i);
        if (PlayerController)
        {
            AACFPlayerController* ACFPlayerController = Cast<AACFPlayerController>(PlayerController);
            if (ACFPlayerController)
            {
                PlayerControllers.Add(ACFPlayerController);
            }
        }
    }

    return PlayerControllers;
}