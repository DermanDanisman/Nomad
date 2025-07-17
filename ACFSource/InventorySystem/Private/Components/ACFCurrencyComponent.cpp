// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Components/ACFCurrencyComponent.h"
#include "ACFItemSystemFunctionLibrary.h"
#include "ARSStatisticsComponent.h"
#include "ARSFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include <Kismet/KismetSystemLibrary.h>
#include <GameFramework/Pawn.h>

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
UACFCurrencyComponent::UACFCurrencyComponent()
{
    // Disable ticking: we only respond to events and RPCs.
    PrimaryComponentTick.bCanEverTick = false;

    // Enable replication of this component and its properties.
    SetIsReplicatedByDefault(true);

    // Start with zero currency.
    CurrencyAmount = 0.f;
}

//------------------------------------------------------------------------------
// Replication setup
//------------------------------------------------------------------------------
void UACFCurrencyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Mark CurrencyAmount for replication and use OnRep_Currency on clients.
    DOREPLIFETIME(UACFCurrencyComponent, CurrencyAmount);
}

//------------------------------------------------------------------------------
// Server RPC: Subtract currency
//------------------------------------------------------------------------------
void UACFCurrencyComponent::RemoveCurrency_Implementation(float Amount)
{
    // Decrease the Amount, clamp so it never goes negative.
    CurrencyAmount -= Amount;
    CurrencyAmount = FMath::Clamp(CurrencyAmount, 0.f, BIG_NUMBER);

    // Notify listeners of the change (negative delta).
    DispatchCurrencyChanged(-Amount);
}

//------------------------------------------------------------------------------
// Server RPC: Set currency directly
//------------------------------------------------------------------------------
void UACFCurrencyComponent::SetCurrency_Implementation(float Amount)
{
    // Compute delta from previous value for broadcast.
    const float delta = CurrencyAmount - Amount;

    // Override the stored value.
    CurrencyAmount = Amount;

    // Notify listeners of the set operation using computed delta.
    DispatchCurrencyChanged(delta);
}

//------------------------------------------------------------------------------
// Server RPC: Add currency
//------------------------------------------------------------------------------
void UACFCurrencyComponent::AddCurrency_Implementation(float Amount)
{
    // Increase the Amount.
    CurrencyAmount += Amount;

    // Broadcast the positive delta.
    DispatchCurrencyChanged(Amount);
}

//------------------------------------------------------------------------------
// BeginPlay
//------------------------------------------------------------------------------
void UACFCurrencyComponent::BeginPlay()
{
    Super::BeginPlay();

    // If configured to drop on death, bind to the health-zero event.
    if (bDropCurrencyOnOwnerDeath && UKismetSystemLibrary::IsServer(this))
    {
        // Look for the statistics component that fires when health hits zero.
        if (auto* statComp = GetOwner()->FindComponentByClass<UARSStatisticsComponent>())
        {
            // Bind once only.
            if (!statComp->OnStatisiticReachesZero.IsAlreadyBound(this, &UACFCurrencyComponent::HandleStatReachedZero))
            {
                statComp->OnStatisiticReachesZero.AddDynamic(this, &UACFCurrencyComponent::HandleStatReachedZero);
            }
        }
    }
}

//------------------------------------------------------------------------------
// Handle a stat reaching zero (e.g. health = 0)
//------------------------------------------------------------------------------
void UACFCurrencyComponent::HandleStatReachedZero(FGameplayTag stat)
{
    // Only drop on server, and only if the stat was health.
    if (UKismetSystemLibrary::IsServer(this) && UARSFunctionLibrary::GetHealthTag() == stat)
    {
        // Apply a small random variation to the drop Amount.
        const float randomVariation = FMath::FRandRange(-CurrencyDropVariation, CurrencyDropVariation);
        const float finalDrop = CurrencyAmount + randomVariation;

        // If there’s enough to drop, spawn a world pickup and remove it.
        if (finalDrop > 1.f)
        {
            if (const APawn* pawn = Cast<APawn>(GetOwner()))
            {
                const FVector spawnLoc = pawn->GetNavAgentLocation();

                // Helper spawns a currency item actor at the pawn’s feet.
                UACFItemSystemFunctionLibrary::SpawnCurrencyItemNearLocation(this, finalDrop, spawnLoc, 100.f);

                // Remove what we dropped from our total.
                RemoveCurrency(finalDrop);
            }
        }
    }
}

//------------------------------------------------------------------------------
// Client-side replication notifier
//------------------------------------------------------------------------------
void UACFCurrencyComponent::OnRep_Currency()
{
    // When CurrencyAmount replicates to a client, broadcast change (delta=0).
    OnCurrencyChanged.Broadcast(CurrencyAmount, 0.f);

    // Allow subclasses or blueprints to react.
    HandleCurrencyChanged();
}

//------------------------------------------------------------------------------
// Broadcast helper
//------------------------------------------------------------------------------
void UACFCurrencyComponent::DispatchCurrencyChanged(float Amount)
{
    // Fire the delegate with new total and delta variation.
    OnCurrencyChanged.Broadcast(CurrencyAmount, Amount);

    // Hook for additional logic in subclasses.
    HandleCurrencyChanged();
}

//------------------------------------------------------------------------------
// Default no-op hook
//------------------------------------------------------------------------------
void UACFCurrencyComponent::HandleCurrencyChanged()
{
    // Intentionally empty; override in Blueprint or child C++ classes.
}
