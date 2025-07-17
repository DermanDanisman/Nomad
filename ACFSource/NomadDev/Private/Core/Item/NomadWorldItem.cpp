// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/Item/NomadWorldItem.h"

#include "Components/ACFStorageComponent.h"
#include "Core/Data/Item/Resource/PickupItemActorData.h"
#include "GameplayTagsManager.h"

ANomadWorldItem::ANomadWorldItem()
{
    // 1) Actor‐level replication
    bReplicates = true;

    // tell the mesh component to replicate its physics state
    ObjectMesh->SetIsReplicated(true);
    ObjectMesh->bReceivesDecals = false;

    // optionally bump your update frequency so you see smoother motion
    NetUpdateFrequency = 66;
    MinNetUpdateFrequency = 33;

    bOnlyRelevantToOwner = false;
    NetCullDistanceSquared = FMath::Square(2000.f); // only replicate within 2km

    // Only let the server do the real work
    if (!PickupItemData)
    {
        return;
    }

    // Pull in your asset data exactly once
    bPickOnOverlap = PickupItemData->GetPickupActorInfo().GetPickOnOverlap();
    bAutoEquipOnPick = PickupItemData->GetPickupActorInfo().GetAutoEquipOnPick();
    OnPickupEffect = PickupItemData->GetPickupActorInfo().GetOnPickupEffect();
    OnPickupBuff = PickupItemData->GetPickupActorInfo().GetOnPickupBuff();
    bDestroyOnGather = PickupItemData->GetPickupActorInfo().GetDestroyAfterGathering();

    // Add items to your storage component
    StorageComponent->AddItems(PickupItemData->PickupActorInfo.Items);

    if (GetItems().Num() > 0)
    {
        SetItemMesh(GetItems().Last());
    }

    if (UStaticMeshComponent* Mesh = ObjectMesh)
    {
        // — this runs on server & clients —
        Mesh->SetCollisionProfileName(FName("Interactable"));
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Mesh->SetCollisionObjectType(ECC_GameTraceChannel16);

        // block everything except Pawn overlaps
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel15, ECR_Ignore);
    }
}

void ANomadWorldItem::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (UStaticMeshComponent* Mesh = ObjectMesh)
    {
        // — this runs on server & clients —
        Mesh->SetCollisionProfileName(FName("Interactable"));
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Mesh->SetCollisionObjectType(ECC_GameTraceChannel16);

        // block everything except Pawn overlaps
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel15, ECR_Ignore);
    }
}

void ANomadWorldItem::StartPhysics()
{
    SetReplicateMovement(true);

    // only run on server authority
    if (!HasAuthority() || !ObjectMesh)
    {
        return;
    }

    // Turn on CCD for this component
    if (auto* BI = ObjectMesh->GetBodyInstance())
    {
        BI->bUseCCD = true;
    }

    ObjectMesh->SetSimulatePhysics(true);

    // give it a random toss:
    const FVector Impulse(
        FMath::RandRange(-150, 150),
        FMath::RandRange(-150, 150),
        FMath::RandRange(50, 100)
        );
    //ObjectMesh->AddImpulse(Impulse, NAME_None, /*bVelChange=*/true);

    // schedule stop in PhysicsWindow seconds:
    GetWorldTimerManager().SetTimer(
        PhysicsStopTimerHandle,
        this,
        &ANomadWorldItem::StopPhysics,
        PhysicsWindow,
        false
        );
}

void ANomadWorldItem::StopPhysics()
{
    if (!HasAuthority() || !ObjectMesh || !ObjectMesh->IsSimulatingPhysics())
    {
        return;
    }

    // Turn on CCD for this component
    if (auto* BI = ObjectMesh->GetBodyInstance())
    {
        BI->bUseCCD = false;
    }

    ObjectMesh->SetSimulatePhysics(false);

    NetUpdateFrequency = 10;
    MinNetUpdateFrequency = 1;

    SetNetDormancy(DORM_DormantAll);

    // snap into final pose (so clients see exact resting state)
    SetActorLocation(GetActorLocation());
    SetActorRotation(GetActorRotation());
}

FText ANomadWorldItem::GetInteractableName_Implementation()
{
    return PickupItemData->GetPickupActorInfo().GetItemName();
}

FGameplayTag ANomadWorldItem::GetCollectionTag_Implementation() const
{
    return PickupItemData->GetPickupActorInfo().GetCollectResourceTag();
}