// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "AMSMapMarkerComponent.generated.h"

class UAMSMarkerWidget;

UCLASS(ClassGroup = (ANS), meta = (BlueprintSpawnableComponent))
class ASCENTMAPSSYSTEM_API UAMSMapMarkerComponent : public UWidgetComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UAMSMapMarkerComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /*Texture to be used to render this marker*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AMS)
    UTexture2D* MarkerTexture;

    /*Useful to categorize and filter the various markers*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AMS)
    FGameplayTag MarkerCategory;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AMS)
    FString MarkerName = "Default Name";

    /*Indicates if this marker should rotate as the referenced actor
    when placed on the map*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AMS)
    bool bShouldRotate = false;

    /*Indicates if we have to enable also the widget in this component in the world */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AMS)
    bool bActivateWorldWidget = true;

    UFUNCTION(BlueprintPure, Category = AMS)
    UAMSMarkerWidget* GetIconWidget() const
    {
        return iconWidget;
    }

public:
    UFUNCTION(BlueprintCallable, Category = AMS)
    void AddMarker();

    UFUNCTION(BlueprintCallable, Category = AMS)
    void RemoveMarker();

    UFUNCTION(BlueprintPure, Category = AMS)
    UTexture2D* GetMarkerTexture() const { return MarkerTexture; }

    UFUNCTION(BlueprintPure, Category = AMS)
    FVector GetOwnerLocation() const;

    UFUNCTION(BlueprintPure, Category = AMS)
    bool IsMarked() const;

    UFUNCTION(BlueprintCallable, Category = AMS)
    void SetMarkerTexture(UTexture2D* val) { MarkerTexture = val; }

    UFUNCTION(BlueprintPure, Category = AMS)
    FGameplayTag GetMarkerCategory() const { return MarkerCategory; }

    UFUNCTION(BlueprintPure, Category = AMS)
    FString GetMarkerName() const
    {
        return MarkerName;
    }

    UFUNCTION(BlueprintCallable, Category = AMS)
    void SetMarkerName(FString inName)
    {
        MarkerName = inName;
    }

    UFUNCTION(BlueprintCallable, Category = AMS)
    void SetMarkerCategory(FGameplayTag val)
    {
        MarkerCategory = val;
    }

    UFUNCTION(BlueprintCallable, Category = AMS)
    void RestoreMarkerCatgory()
    {
        MarkerCategory = originalCategory;
    }

    UFUNCTION(BlueprintCallable, Blueprintpure, Category = AMS)
    FRotator GetOwnerRotation() const;

    UFUNCTION(BlueprintCallable, Blueprintpure, Category = AMS)
    bool GetShouldRotate() const { return bShouldRotate; }

    UFUNCTION(BlueprintCallable, Category = AMS)
    void SetShouldRotate(bool val) { bShouldRotate = val; }

    UFUNCTION(BlueprintPure, Category = AMS)
    bool GetActivateWorldWidget() const { return bActivateWorldWidget; }

    UFUNCTION(BlueprintCallable, Category = AMS)
    void SetActivateWorldWidget(bool val) { bActivateWorldWidget = val; }

private:
    TObjectPtr<UAMSMarkerWidget> iconWidget;

    FGameplayTag originalCategory;
};
