// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Items/ACFPickup.h"
#include "ARSStatisticsComponent.h"
#include <GameFramework/Pawn.h>

// Sets default values
AACFPickup::AACFPickup()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AACFPickup::BeginPlay()
{
    Super::BeginPlay();
}

void AACFPickup::OnInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType /*= ""*/)
{
    if (Pawn)
    {
        UARSStatisticsComponent* statComp = Pawn->FindComponentByClass<UARSStatisticsComponent>();
        if (statComp)
        {
            for (const auto& stat : OnPickupEffect)
            {
                statComp->ModifyStat(stat);
            }
            for (const auto& att : OnPickupBuff)
            {
                statComp->AddTimedAttributeSetModifier(att.Modifier, att.Duration);
            }
        }
    }
}

void AACFPickup::OnInteractableRegisteredByPawn_Implementation(class APawn* Pawn)
{
    if (HasAuthority() && bPickOnOverlap && Execute_CanBeInteracted(this, Pawn))
    {
        Execute_OnInteractedByPawn(this, Pawn, "");
    }
}