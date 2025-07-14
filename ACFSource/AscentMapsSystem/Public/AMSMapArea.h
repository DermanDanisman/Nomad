// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "AMSTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "AMSMapArea.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ASCENTMAPSSYSTEM_API AAMSMapArea : public AActor {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AAMSMapArea();

    UFUNCTION(BlueprintCallable, Category = AMS)
    FName GetAreaName() const
    {
        return AreaName;
    }

    /*Returns the texture to be used to render this area in the map,
    depending on current configuration*/
    UFUNCTION(BlueprintPure, Category = AMS)
    UTexture* GetMapTexture() const;

    /*Returns the world location of the center of the map bounds*/
    UFUNCTION(BlueprintPure, Category = AMS)
    FVector GetMapAreaCenter() const;

    UFUNCTION(BlueprintPure, Category = AMS)
    FVector GetMapAreaExtent() const;

    UFUNCTION(BlueprintPure, Category = AMS)
    FVector2D GetMapAreaTopLeftmostPoint() const;

    UFUNCTION(BlueprintPure, Category = AMS)
    FVector2D GetMapAreaBottomRightmostPoint() const;

    UFUNCTION(BlueprintCallable, Category = AMS)
    FVector2D GetNormalized2DPositionFromWorldLocation(const FVector& worldPos) const;

    UFUNCTION(BlueprintCallable, Category = AMS)
    bool IsPointInThisArea(const FVector& point) const;

    UFUNCTION(BlueprintCallable, Category = AMS)
    FVector GetPointRelativeLocation(const FVector& point) const;

    /*Generates the textures in the render target for the current area*/
    UFUNCTION(BlueprintCallable, Category = AMS)
    void GenerateMapTexture();

    /*Generates the textures in the render target for the current area*/
    UFUNCTION(BlueprintCallable, Category = AMS)
    void GenerateMap(const TArray<AActor*>& ActorsToHide, float ExposuteCompensation = 1.f);

    /*Spawns a marker actor of the provided class inside the relative normalized location
    inside the map area with 0, 0 being the left topmost point and 1 , 1 the right bottom point
    of the area. Returns the spawned instance*/
    UFUNCTION(BlueprintCallable, Category = AMS)
    class AAMSActorMarker* SpawnActorMarkerAtMapLocation(const FVector2D& normalizedMapPosition, const TSubclassOf<class AAMSActorMarker>& markerClass);

    UFUNCTION(BlueprintPure, Category = AMS)
    FVector GetWorldLocationFromNormalized2DPosition(const FVector2D& normalizedPosition) const;

    UFUNCTION(BlueprintCallable, Category = AMS)
    class USceneCaptureComponent2D* GetCaptureComponent()
    {
        return CaptureComp;
    }

    UFUNCTION(BlueprintPure, Category = AMS)
    class UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

    void SetRenderTarget(TObjectPtr<class UTextureRenderTarget2D> val) { RenderTarget = val; }

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AMS)
    FName AreaName;

    UPROPERTY(EditAnywhere, Category = AMS)
    int32 AreaPriority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AMS)
    EAMSBackgroundType TextureType;

    UPROPERTY(EditAnywhere, Category = AMS)
    float AreaSize = 20000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "TextureType == EAMSBackgroundType::ERenderTarget"), Category = AMS)
    TObjectPtr<class UTextureRenderTarget2D> RenderTarget;

    UPROPERTY(EditAnywhere, meta = (EditCondition = "TextureType == EAMSBackgroundType::ECustomTexture"), Category = AMS)
    TObjectPtr<class UTexture2D> Texture;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AMS)
    TObjectPtr<class USceneCaptureComponent2D> CaptureComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = AMS)
    TObjectPtr<class UBoxComponent> MapBounds;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

private:
    void UpdateBoxProperties();

   
    UFUNCTION()
    void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
