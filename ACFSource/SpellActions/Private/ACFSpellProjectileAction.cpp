// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ACFSpellProjectileAction.h"
#include "ACFItemSystemFunctionLibrary.h"
#include "ATSBaseTargetComponent.h"
#include "Components/ACFShootingComponent.h"
#include "Items/ACFProjectile.h"
#include <GameFramework/Pawn.h>
#include <Kismet/KismetMathLibrary.h>

UACFSpellProjectileAction::UACFSpellProjectileAction()
{
    LaunchSocketNames.Add("RightHandSocket");
    LaunchSocketNames.Add("LeftHandSocket");
    ShootDirection = EShootDirection::EControllerDirection;
}

void UACFSpellProjectileAction::OnNotablePointReached_Implementation()
{
    Super::OnNotablePointReached_Implementation();

    if (CharacterOwner && ProjectileClass) {
        UACFShootingComponent* shootComp = CharacterOwner->FindComponentByClass<UACFShootingComponent>();
        if (shootComp) {
            const FName socketName = GetDesiredSocketName();
            FRotator direction;

            switch (ShootDirection) {
            case EShootDirection::EControllerDirection:
                direction = CharacterOwner->GetControlRotation();
                break;
            case EShootDirection::ECurrentTarget:
                if (CharacterOwner->GetController()) {
                    const UATSBaseTargetComponent* targetComp = CharacterOwner->GetController()->FindComponentByClass<UATSBaseTargetComponent>();
                    if (targetComp && targetComp->GetCurrentTarget()) {
                        direction = UKismetMathLibrary::FindLookAtRotation(CharacterOwner->GetMesh()->GetSocketLocation(socketName), targetComp->GetCurrentTarget()->GetActorLocation());
                        break;
                    }
                }
                direction = CharacterOwner->GetControlRotation();

                break;
            }

            shootComp->ShootAtDirection(direction, 1.f, ProjectileClass, socketName);
        } else {
            UE_LOG(LogTemp, Warning, TEXT("No shootingComp on your character! - UACFSpellProjectileAction"));
        }
    } else {
        UE_LOG(LogTemp, Warning, TEXT("No shooting COmp on your character! - UACFSpellProjectileAction"));
    }
}

FName UACFSpellProjectileAction::GetDesiredSocketName() const
{
    const int32 index = GetComboCounter();
    if (LaunchSocketNames.IsValidIndex(index)) {
        return LaunchSocketNames[index];
    }
    return NAME_None;
}
