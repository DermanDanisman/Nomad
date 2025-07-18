// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/FunctionLibrary/NomadItemSystemFunctionLibrary.h"

#include "Core/Item/NomadWorldItem.h"
#include "Items/ACFWorldItem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NomadDev/NomadDev.h"

AACFWorldItem* UNomadItemSystemFunctionLibrary::SpawnResourceWorldItemNearLocation(UObject* WorldContextObject,
    const TArray<FBaseItem>& ContainedItem, const FVector& Location, const float AcceptanceRadius, const bool bUsePhysics, UPickupItemActorData* ItemActorData)
{
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid WorldContextObject!"));
        return nullptr;
    }

    const FTransform SpawnTransform = FTransform(Location);
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ANomadWorldItem* NomadWorldItem = World->SpawnActorDeferred<ANomadWorldItem>(
        ANomadWorldItem::StaticClass(),
        SpawnTransform,
        /*Owner=*/ nullptr,
        /*Instigator=*/ nullptr,
        SpawnParams.SpawnCollisionHandlingOverride
        );

    if (!NomadWorldItem)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn NomadWorldItem!"));
        return nullptr;
    }

    for (const FBaseItem& Item : ContainedItem)
    {
        NomadWorldItem->AddItem(Item);
        NomadWorldItem->SetPickupItemData(ItemActorData);
    }

    // Complete spawn
    UGameplayStatics::FinishSpawningActor(NomadWorldItem, SpawnTransform);

    if (bUsePhysics)
    {
        const UStaticMeshComponent* MeshComp = NomadWorldItem->FindComponentByClass<UStaticMeshComponent>();
        if (MeshComp)
        {
            // delay StartPhysics so clients have time to replicate the new actor
            NomadWorldItem->GetWorldTimerManager().SetTimer(
                NomadWorldItem->PhysicsStartTimerHandle,
                NomadWorldItem,
                &ANomadWorldItem::StartPhysics,
                ANomadWorldItem::StartDelay,
                false
                );
        }
    }

    return NomadWorldItem;
}

FHitResult UNomadItemSystemFunctionLibrary::PerformLineTraceFromCameraManager(UObject* WorldContextObject,
    APlayerController* PlayerController, const float TraceLength, bool bShowDebug)
{
    const UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World || !PlayerController || !PlayerController->PlayerCameraManager)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid WorldContextObject or CameraComponent!"));
        return FHitResult();
    }

    const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
    const FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
    const FVector CameraForward = CameraRotation.Vector();

    const FVector TraceStart = CameraLocation;
    const FVector TraceEnd = CameraLocation + CameraForward * TraceLength;

    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(PlayerController->GetPawn());

    // Test both channels and find the closest hit
    FHitResult InteractableHit;
    FHitResult GatherableHit;

    bool bInteractableHit = World->LineTraceSingleByChannel(
        InteractableHit, TraceStart, TraceEnd, ECC_Interactable, CollisionParams);

    bool bGatherableHit = World->LineTraceSingleByChannel(
        GatherableHit, TraceStart, TraceEnd, ECC_Gatherable, CollisionParams);

    // Determine which hit is closer (if any)
    FHitResult FinalHit;
    bool bAnyHit = false;

    if (bInteractableHit && bGatherableHit)
    {
        // Both hit something, choose the closer one
        float InteractableDistance = FVector::Dist(TraceStart, InteractableHit.Location);
        float GatherableDistance = FVector::Dist(TraceStart, GatherableHit.Location);

        if (InteractableDistance <= GatherableDistance)
        {
            FinalHit = InteractableHit;
            bAnyHit = true;
            UE_LOG(LogTemp, Log, TEXT("Hit Interactable: %s"),
                FinalHit.GetActor() ? *FinalHit.GetActor()->GetName() : TEXT("None"));
        }
        else
        {
            FinalHit = GatherableHit;
            bAnyHit = true;
            UE_LOG(LogTemp, Log, TEXT("Hit Gatherable: %s"),
                FinalHit.GetActor() ? *FinalHit.GetActor()->GetName() : TEXT("None"));
        }
    }
    else if (bInteractableHit)
    {
        FinalHit = InteractableHit;
        bAnyHit = true;
        UE_LOG(LogTemp, Log, TEXT("Hit Interactable: %s"),
            FinalHit.GetActor() ? *FinalHit.GetActor()->GetName() : TEXT("None"));
    }
    else if (bGatherableHit)
    {
        FinalHit = GatherableHit;
        bAnyHit = true;
        UE_LOG(LogTemp, Log, TEXT("Hit Gatherable: %s"),
            FinalHit.GetActor() ? *FinalHit.GetActor()->GetName() : TEXT("None"));
    }

    if (bShowDebug)
    {
        FColor LineColor = bAnyHit ? FColor::Green : FColor::Red;
        DrawDebugLine(World, TraceStart, TraceEnd, LineColor, false, 1.0f, 0, 2.0f);

        if (bAnyHit)
        {
            // Color code based on type
            FColor HitColor = FColor::Yellow;
            if (bInteractableHit && FinalHit.GetActor() == InteractableHit.GetActor())
            {
                HitColor = FColor::Blue; // Blue for Interactable
            }
            else if (bGatherableHit && FinalHit.GetActor() == GatherableHit.GetActor())
            {
                HitColor = FColor::Orange; // Orange for Gatherable
            }

            DrawDebugSphere(World, FinalHit.Location, 5.0f, 12, HitColor, false, 1.0f);
        }
    }

    return FinalHit;
}