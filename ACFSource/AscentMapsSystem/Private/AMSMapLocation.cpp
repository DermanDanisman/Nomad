// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AMSMapLocation.h"
#include "AMSMapMarkerComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include <GameFramework/Pawn.h>

// Sets default values
AAMSMapLocation::AAMSMapLocation()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;
    SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("Discover Area"));
    MarkerComp = CreateDefaultSubobject<UAMSMapMarkerComponent>(TEXT("Marker Componenr"));
    SphereComp->SetSphereRadius(2000.f);
}

FString AAMSMapLocation::GetLocationName() const
{
    return GetMarkerComponent()->GetMarkerName();
}

void AAMSMapLocation::SetLocationName(const FString& newName)
{
    GetMarkerComponent()->SetMarkerName(newName);
}

void AAMSMapLocation::SetDiscoveredState(bool newState)
{
    bDiscovered = newState;
    if (bDiscovered) {
        MarkerComp->AddMarker();
        SphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    } else {
        MarkerComp->RemoveMarker();
    }
}

// Called when the game starts or when spawned
void AAMSMapLocation::BeginPlay()
{
    Super::BeginPlay();
    if (!IsDiscovered() && bDiscoverOnPlayerOverlap) {
        SphereComp->OnComponentBeginOverlap.AddDynamic(this, &AAMSMapLocation::HandleLocalPlayerOverlap);
    } else {
        SphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void AAMSMapLocation::OnDiscovered_Implementation()
{
}

void AAMSMapLocation::HandleLocalPlayerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (UGameplayStatics::GetPlayerPawn(this, 0) == OtherActor && !IsDiscovered()) {
        DiscoverLocation();
    }
}

void AAMSMapLocation::DiscoverLocation()
{
    SetDiscoveredState(true);
    OnDiscovered();
    OnDiscoveredEvent.Broadcast();
}

void AAMSMapLocation::OnTeleported_Implementation()
{
    APawn* currentPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (currentPawn) {
        FHitResult outRes;
        currentPawn->SetActorLocation(GetFastTravelPoint(), false, &outRes, ETeleportType::TeleportPhysics);
    }
}

bool AAMSMapLocation::CanFastTravel_Implementation() const
{
    return bCanFastTravel;
}

void AAMSMapLocation::FastTraverlToLocation()
{
    if (CanFastTravel()) {
        OnTeleported();
        OnFastTravelEvent.Broadcast();
    }
}

FVector AAMSMapLocation::GetFastTravelPoint() const
{
    return GetActorLocation() + FastTravelLocation;
}
