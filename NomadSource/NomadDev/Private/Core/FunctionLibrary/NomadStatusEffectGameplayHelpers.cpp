// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/FunctionLibrary/NomadStatusEffectGameplayHelpers.h"

#include "ARSStatisticsComponent.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Components/ACFCharacterMovementComponent.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromStat(ACharacter* Character)
{
    if (!Character) return;
    const UARSStatisticsComponent* StatsComp = Character->FindComponentByClass<UARSStatisticsComponent>();
    UACFCharacterMovementComponent* MoveComp = Character->FindComponentByClass<UACFCharacterMovementComponent>();
    if (!StatsComp || !MoveComp) return;

    const float NewSpeed = StatsComp->GetCurrentAttributeValue(FGameplayTag::RequestGameplayTag(TEXT("RPG.Attributes.MovementSpeed")));
    if (NewSpeed > 0.f)
    {
        MoveComp->MaxWalkSpeed = NewSpeed;
        // Optionally: MoveComp->CharacterMaxSpeed = NewSpeed;
    }
}

bool UNomadStatusEffectGameplayHelpers::IsSprintBlocked(ACharacter* Character)
{
    {
        static const FGameplayTag SprintBlockTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Sprint"));
        if (!Character) return false;
        auto* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>();
        return (SEManager && SEManager->HasBlockingTag(SprintBlockTag));
    }
}