// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/ACFDamageHandlerComponent.h"
#include "ARSStatisticsComponent.h"
#include "ARSTypes.h"
#include "Actors/ACFCharacter.h"
#include "Components/ACFTeamManagerComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "Game/ACFDamageType.h"
#include "Game/ACFDamageTypeCalculator.h"
#include "Game/ACFFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include <Components/MeshComponent.h>
#include <Engine/EngineTypes.h>
#include <GameFramework/Actor.h>
#include <GameFramework/Controller.h>
#include <GameFramework/DamageType.h>
#include <Kismet/KismetSystemLibrary.h>
#include <PhysicsEngine/BodyInstance.h>
#include "GameplayTagContainer.h"

// Sets default values for this component's properties
UACFDamageHandlerComponent::UACFDamageHandlerComponent()
{
    // Component does not tick by default for performance
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    bIsAlive = true;
    // Default damage calculator is the standard ACF damage type calculator
    DamageCalculatorClass = UACFDamageTypeCalculator::StaticClass();
}

void UACFDamageHandlerComponent::InitializeDamageCollisions(ETeam inCombatTeam)
{
    // Prevent duplicate initialization or unnecessary channel assignment if already set for this team
    if (bInit && inCombatTeam == combatTeam)
    {
        return;
    }

    // Get the team manager from the world and assign the correct collision channel for this team
    UACFTeamManagerComponent* TeamManager = UACFFunctionLibrary::GetACFTeamManager(GetWorld());
    if (TeamManager)
    {
        combatTeam = inCombatTeam;
        AssignCollisionProfile(TeamManager->GetCollisionChannelByTeam(combatTeam, bUseBlockingCollisionChannel));

        OnTeamChanged.Broadcast(combatTeam);
        bInit = true;
    } else
    {
        UE_LOG(LogTemp, Error, TEXT("Remember to add an ACFGameState to your World!!! -  UACFDamageHandlerComponent::InitializeDamageCollisions"));
    }
}

void UACFDamageHandlerComponent::AssignCollisionProfile(const TEnumAsByte<ECollisionChannel> channel)
{
    // Assign the specified collision channel to all mesh components on the owning actor
    TArray<UActorComponent*> meshes;
    GetOwner()->GetComponents(UMeshComponent::StaticClass(), meshes);

    for (auto& mesh : meshes)
    {
        UMeshComponent* meshComp = Cast<UMeshComponent>(mesh);

        if (meshComp)
        {
            meshComp->SetCollisionObjectType(channel);
        }
    }
}

void UACFDamageHandlerComponent::BeginPlay()
{
    Super::BeginPlay();
    // Bind to the OnStatisiticReachesZero delegate for health/stat depletion events
    UARSStatisticsComponent* StatisticsComp = GetOwner()->FindComponentByClass<UARSStatisticsComponent>();
    if (StatisticsComp && !StatisticsComp->OnStatisiticReachesZero.IsAlreadyBound(this, &UACFDamageHandlerComponent::HandleStatReachedZero))
    {
        StatisticsComp->OnStatisiticReachesZero.AddDynamic(this, &UACFDamageHandlerComponent::HandleStatReachedZero);
    }
}

float UACFDamageHandlerComponent::TakeDamage(class AActor* damageReceiver, float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // Null check for receiver
    if (!damageReceiver)
    {
        return Damage;
    }

    // Extract hit result and shot direction from the incoming DamageEvent
    FHitResult outDamage;
    FVector ShotDirection;
    DamageEvent.GetBestHitInfo(damageReceiver, DamageCauser, outDamage, ShotDirection);

    // Construct and process the FACFDamageEvent with all relevant details
    FACFDamageEvent outDamageEvent;
    ConstructDamageReceived(damageReceiver, Damage, EventInstigator, outDamage.Location, outDamage.Component.Get(), outDamage.BoneName, ShotDirection, DamageEvent.DamageTypeClass,
        DamageCauser);

    // Get stats component and apply the final calculated damage as a stat modification
    UARSStatisticsComponent* StatisticsComp = damageReceiver->FindComponentByClass<UARSStatisticsComponent>();

    if (StatisticsComp)
    {
        FStatisticValue statMod(UACFFunctionLibrary::GetHealthTag(), -LastDamageReceived.FinalDamage);
        StatisticsComp->ModifyStat(statMod);
    }
    // Notify clients about the received damage for replication and event triggers
    ClientsReceiveDamage(LastDamageReceived);
    return LastDamageReceived.FinalDamage;
}

void UACFDamageHandlerComponent::Revive_Implementation()
{
    bIsAlive = true;

    // Restart stat regeneration on revive
    UARSStatisticsComponent* StatisticsComp = GetOwner()->FindComponentByClass<UARSStatisticsComponent>();
    if (StatisticsComp)
    {
        StatisticsComp->StartRegeneration();
    }
}

void UACFDamageHandlerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Ensure alive state is replicated
    DOREPLIFETIME(UACFDamageHandlerComponent, bIsAlive);
}

void UACFDamageHandlerComponent::ConstructDamageReceived(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation,
    class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, TSubclassOf<UDamageType> DamageType,
    AActor* DamageCauser)
{
    // Populate FACFDamageEvent with hit, direction, tags, and actor references
    FACFDamageEvent TempDamageEvent;
    TempDamageEvent.ContextString = NAME_None;
    TempDamageEvent.FinalDamage = Damage;
    TempDamageEvent.HitDirection = ShotFromDirection;
    if (ShotFromDirection == FVector(0,0,0))
        TempDamageEvent.HitDirection = DamageCauser->GetActorLocation();
    TempDamageEvent.HitResult.BoneName = BoneName;
    TempDamageEvent.HitResult.ImpactPoint = HitLocation;
    TempDamageEvent.HitResult.Location = HitLocation;
    TempDamageEvent.HitResult.HitObjectHandle = FActorInstanceHandle(DamagedActor);
    TempDamageEvent.DamageReceiver = DamagedActor;
    TempDamageEvent.DamageClass = DamageType;
    if (DamageCauser)
    {
        TempDamageEvent.DamageDealer = DamageCauser;
        TempDamageEvent.DamageDirection = UACFFunctionLibrary::GetHitDirectionByHitResult(TempDamageEvent.DamageDealer, TempDamageEvent.HitResult);
    }

    TempDamageEvent.HitResponseAction = FGameplayTag();

    // Append tags from the DamageType asset to the damage event for resistance/filtering logic
    if (DamageType)
    {
        UACFDamageType* DamageTypeCDO = DamageType->GetDefaultObject<UACFDamageType>();
        if (DamageTypeCDO)
        {
            TempDamageEvent.DamageTags.AppendTags(DamageTypeCDO->DamageTags);
        }
    }

    /*
    // Example: Add per-hit/context tags here if needed (backstab, critical, buffs, etc.)
    if (bIsBackstab) {
        tempDamageEvent.DamageTags.AddTag(FGameplayTag::RequestGameplayTag("Attack.Backstab"));
    }
    if (bIsCritical) {
        tempDamageEvent.DamageTags.AddTag(FGameplayTag::RequestGameplayTag("Attack.Critical"));
    }
    if (AttackerHasBuff) {
        tempDamageEvent.DamageTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Berserk"));
    }
    // ...etc
    */

    // If the damaged actor is an ACF character, get damage zone and physical material
    AACFCharacter* acfReceiver = Cast<AACFCharacter>(DamagedActor);

    if (acfReceiver)
    {
        TempDamageEvent.DamageZone = acfReceiver->GetDamageZoneByBoneName(BoneName);
        FBodyInstance* bodyInstance = acfReceiver->GetMesh()->GetBodyInstance(BoneName);
        if (bodyInstance)
        {
            TempDamageEvent.PhysMaterial = bodyInstance->GetSimplePhysicalMaterial();
        }
    }

    // Use the damage calculator to evaluate hit response, critical state, and recalculate final damage
    if (DamageCalculatorClass)
    {
        if (!DamageCalculator)
        {
            DamageCalculator = NewObject<UACFDamageCalculation>(this, DamageCalculatorClass);
        }
        TempDamageEvent.HitResponseAction = DamageCalculator->EvaluateHitResponseAction(TempDamageEvent, HitResponseActions);
        TempDamageEvent.bIsCritical = DamageCalculator->IsCriticalDamage(TempDamageEvent);
        TempDamageEvent.FinalDamage = DamageCalculator->CalculateFinalDamage(TempDamageEvent);
    } else
    {
        ensure(false);
        UE_LOG(LogTemp, Error, TEXT("MISSING DAMAGE CALCULATOR CLASS -  UACFDamageHandlerComponent"));
    }

    // Store the fully processed damage event
    LastDamageReceived = TempDamageEvent;
}

void UACFDamageHandlerComponent::HandleStatReachedZero(FGameplayTag stat)
{
    // Called when a monitored stat (health) reaches zero
    if (UACFFunctionLibrary::GetHealthTag() == stat)
    {
        if (GetOwner()->HasAuthority())
        {
            // Stop regeneration and award EXP to killer if applicable
            UARSStatisticsComponent* StatisticsComp = GetOwner()->FindComponentByClass<UARSStatisticsComponent>();
            if (StatisticsComp)
            {
                StatisticsComp->StopRegeneration();
                if (LastDamageReceived.DamageDealer)
                {
                    UARSStatisticsComponent* dealerStatComp = LastDamageReceived.DamageDealer->FindComponentByClass<UARSStatisticsComponent>();
                    if (dealerStatComp)
                    {
                        dealerStatComp->AddExp(StatisticsComp->GetExpOnDeath());
                    }
                }
            }
        }
        bIsAlive = false;
        // Broadcast the death event for Blueprint/C++ listeners
        OnOwnerDeath.Broadcast();
    }
}

void UACFDamageHandlerComponent::ClientsReceiveDamage_Implementation(const FACFDamageEvent& damageEvent)
{
    // Replicate the damage event to clients and broadcast the OnDamageReceived event
    LastDamageReceived = damageEvent;
    OnDamageReceived.Broadcast(damageEvent);
}