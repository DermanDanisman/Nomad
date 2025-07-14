// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/Item/NomadPickupItem.h"

#include "Components/ACFStorageComponent.h"

ANomadPickupItem::ANomadPickupItem() {}

void ANomadPickupItem::BeginPlay()
{
    Super::BeginPlay();

    // Only let the server do the real work
    if (!PickupItemData)
    {
        return;
    }

    // Pull in your asset data exactly once
    bPickOnOverlap = PickupItemData->PickupActorInfo.bPickOnOverlap;
    bAutoEquipOnPick = PickupItemData->PickupActorInfo.bAutoEquipOnPick;
    OnPickupEffect = PickupItemData->PickupActorInfo.OnPickupEffect;
    OnPickupBuff = PickupItemData->PickupActorInfo.OnPickupBuff;
    bDestroyOnGather = PickupItemData->PickupActorInfo.bDestroyAfterGathering;

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

void ANomadPickupItem::OnInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType)
{
    if (Pawn)
    {
        UACFEquipmentComponent* EquipComp = Pawn->FindComponentByClass<UACFEquipmentComponent>();
        if (EquipComp && StorageComponent)
        {
            StorageComponent->MoveItemsToInventory(GetItems(), EquipComp);
            StorageComponent->GatherCurrency(StorageComponent->GetCurrentCurrencyAmount(), StorageComponent->GetPawnCurrencyComponent(Pawn));
        }
        if (bDestroyOnGather)
        {
            Destroy();
        }
    }
}