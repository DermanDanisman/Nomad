// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AMSMapArea.h"
#include "AMSActorMarker.h"
#include "AMSMapSubsystem.h"
#include "AMSTypes.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include <Engine/GameInstance.h>
#include <GameFramework/Pawn.h>

// Sets default values
AAMSMapArea::AAMSMapArea()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;
    CaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Scene Capture"));
    MapBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Box Component"));
    SetRootComponent(MapBounds);
    CaptureComp->SetupAttachment(MapBounds);
    CaptureComp->bCaptureEveryFrame = false;

    MapBounds->OnComponentBeginOverlap.AddDynamic(this, &AAMSMapArea::OnBeginOverlap);
    MapBounds->OnComponentEndOverlap.AddDynamic(this, &AAMSMapArea::OnEndOverlap);
    MapBounds->InitBoxExtent(FVector(AreaSize));

    CaptureComp->ProjectionType = ECameraProjectionMode::Type::Orthographic;
    CaptureComp->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    TextureType = EAMSBackgroundType::ECustomTexture;
    UpdateBoxProperties();
}

UTexture* AAMSMapArea::GetMapTexture() const
{
    if (TextureType == EAMSBackgroundType::ERenderTarget) {
        return CaptureComp->TextureTarget;
    } else {
        return Texture;
    }
}

FVector AAMSMapArea::GetMapAreaCenter() const
{
    return MapBounds->GetComponentLocation();
}

FVector AAMSMapArea::GetMapAreaExtent() const
{
    return MapBounds->GetScaledBoxExtent();
}

FVector2D AAMSMapArea::GetMapAreaTopLeftmostPoint() const
{
    return FVector2D(GetMapAreaCenter().X - GetMapAreaExtent().X,
        GetMapAreaCenter().Y - GetMapAreaExtent().Y);
}

FVector2D AAMSMapArea::GetMapAreaBottomRightmostPoint() const
{
    return FVector2D(GetMapAreaCenter().X + GetMapAreaExtent().X,
        GetMapAreaCenter().Y + GetMapAreaExtent().Y);
}

FVector AAMSMapArea::GetWorldLocationFromNormalized2DPosition(const FVector2D& normalizedPosition) const
{
    FVector outVector;

    outVector.X = normalizedPosition.X * GetMapAreaExtent().X * 2;
    outVector.Y = normalizedPosition.Y * GetMapAreaExtent().Y * 2;

    const FVector2D leftmost = GetMapAreaTopLeftmostPoint();
    outVector.X += leftmost.X;
    outVector.Y += leftmost.Y;

    outVector.Z = MapBounds->GetComponentLocation().Z;
    // 	UKismetSystemLibrary::LineTraceSingleForObjects(this, GetT)
    // 	outVector.Z =
    ensure(IsPointInThisArea(outVector));
    return outVector;
}

FVector2D AAMSMapArea::GetNormalized2DPositionFromWorldLocation(const FVector& worldPos) const
{
    FVector2D outPos;
    //     const FVector relativePos = GetPointRelativeLocation(worldPos);
    //
    //     outPos.X = relativePos.X / GetMapAreaExtent().X;
    //     outPos.Y = relativePos.Y / GetMapAreaExtent().Y;

    const FVector2D leftmost = GetMapAreaTopLeftmostPoint();
    const FVector2D rightmost = GetMapAreaBottomRightmostPoint();

    outPos.X = UKismetMathLibrary::NormalizeToRange(worldPos.X, leftmost.X,
        rightmost.X);

    outPos.Y = UKismetMathLibrary::NormalizeToRange(worldPos.Y, leftmost.Y,
        rightmost.Y);

    return outPos;
}

bool AAMSMapArea::IsPointInThisArea(const FVector& point) const
{
    return UKismetMathLibrary::IsPointInBox(point, GetMapAreaCenter(), GetMapAreaExtent());
}

FVector AAMSMapArea::GetPointRelativeLocation(const FVector& point) const
{
    return point - GetMapAreaCenter();
}

// Called when the game starts or when spawned
void AAMSMapArea::BeginPlay()
{
    Super::BeginPlay();
    CaptureComp->bCaptureEveryFrame = false;
    if (TextureType == EAMSBackgroundType::ERenderTarget) {
        CaptureComp->TextureTarget = GetRenderTarget();
        GenerateMapTexture();
    }

    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    UAMSMapSubsystem* mapSubsystem = GameInstance->GetSubsystem<UAMSMapSubsystem>();
    if (mapSubsystem) {
        mapSubsystem->RegisterMapArea(AreaName, this);
        const APawn* currentPawn = UGameplayStatics::GetPlayerPawn(this, 0);
        if (currentPawn) {
            TArray<AActor*> overlappingActors;
            MapBounds->GetOverlappingActors(overlappingActors, currentPawn->StaticClass());
            if (overlappingActors.Contains(currentPawn)) {
                mapSubsystem->RegisterPlayerInArea(AreaName, AreaPriority);
            }
        }
    }
}

#if WITH_EDITOR
void AAMSMapArea::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    UpdateBoxProperties();
}
#endif

void AAMSMapArea::UpdateBoxProperties()
{
    MapBounds->SetBoxExtent(FVector(AreaSize));
    const float lineTickness = FMath::Clamp(5.0f, 500.f, AreaSize / 100.f);
    MapBounds->SetLineThickness(lineTickness);
    CaptureComp->SetRelativeLocation(FVector(0, 0, AreaSize));
    CaptureComp->SetWorldRotation(FRotator(-90, -90, 0));
    CaptureComp->OrthoWidth = AreaSize * 2;
}

void AAMSMapArea::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    const APawn* currentPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (currentPawn) {
        if (OtherActor == currentPawn) {
            UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);

            UAMSMapSubsystem* mapSubsystem = GameInstance->GetSubsystem<UAMSMapSubsystem>();

            mapSubsystem->RegisterPlayerInArea(AreaName, AreaPriority);
        }
    }
}

void AAMSMapArea::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    const APawn* currentPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (currentPawn) {
        if (OtherActor == currentPawn) {
            UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
            UAMSMapSubsystem* mapSubsystem = GameInstance->GetSubsystem<UAMSMapSubsystem>();
            mapSubsystem->UnregisterPlayerInArea(AreaName);
        }
    }
}

void AAMSMapArea::GenerateMapTexture()
{
    CaptureComp->CaptureScene();
}

void AAMSMapArea::GenerateMap(const TArray<AActor*>& ActorsToHide, float ExposuteCompensation /*= 1.f*/)
{
    CaptureComp->HiddenActors = ActorsToHide;
    CaptureComp->PostProcessSettings.AutoExposureBias = ExposuteCompensation;
    GenerateMapTexture();
}

// TODO: IMPLEMENT ME
AAMSActorMarker* AAMSMapArea::SpawnActorMarkerAtMapLocation(const FVector2D& normalizedMapPosition, const TSubclassOf<AAMSActorMarker>& markerClass)
{
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    UAMSMapSubsystem* mapSubsystem = GameInstance->GetSubsystem<UAMSMapSubsystem>();
    const FVector worldPos = GetWorldLocationFromNormalized2DPosition(normalizedMapPosition);


    return mapSubsystem->SpawnMarkerActor(markerClass, worldPos);
}

