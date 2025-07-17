// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/Game/NomadDevGameMode.h"

#include "Core/Player/NomadPlayerController.h"
#include "Core/Player/NomadPlayerState.h"
#include "Game/ACFPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "UObject/ConstructorHelpers.h"

ANomadDevGameMode::ANomadDevGameMode()
{
    // set default pawn class to our Blueprinted character
    /*
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
    if (PlayerPawnBPClass.Class != NULL)
    {
	DefaultPawnClass = PlayerPawnBPClass.Class;
    }*/

    bUseSeamlessTravel = true;
}

void ANomadDevGameMode::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
    Super::GetSeamlessTravelActorList(bToTransition, ActorList);

    /*
    for (auto Actor : ActorList)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(1, 30.f, FColor::Blue,
                FString::Printf(TEXT("Nomad Dev Game Mode: Seamless Travel Actor List: %s"), *Actor->GetName()));
        }
    }*/
}

void ANomadDevGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
    Super::HandleSeamlessTravelPlayer(C);
    
    if (ANomadPlayerController* NC = Cast<ANomadPlayerController>(C))
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, 
                FString::Printf(TEXT("HandleSeamlessTravelPlayer called for: %s"), 
                NC ? *NC->GetName() : TEXT("NULL")));
        }
        
        GetWorldTimerManager().SetTimer(
        DelayedCustomizationHandle,
        [this, NC]()
        {
            // This is the same instance you carried over!
            NC->Execute_BP_ApplyCustomizationState(NC);
        },
       2.0f,  // 1-second delay
       false
       );
    }

    DelayedCustomizationHandle.Invalidate();
}

void ANomadDevGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (ANomadPlayerController* PlayerControllerLocal = Cast<ANomadPlayerController>(NewPlayer))
    {
        PlayerController.Add(PlayerControllerLocal);
    }
}

void ANomadDevGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
}

void ANomadDevGameMode::StartPlay()
{
    Super::StartPlay();

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(1, 30.f, FColor::Blue,
            FString::Printf(TEXT("ANomadDevGameMode::StartPlay()")));
    }
}

void ANomadDevGameMode::GameWelcomePlayer(UNetConnection* Connection, FString& RedirectURL)
{
    Super::GameWelcomePlayer(Connection, RedirectURL);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(1, 30.f, FColor::Blue,
            FString::Printf(TEXT("ANomadDevGameMode::GameWelcomePlayer")));
    }
}


