// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AMSActorMarker.generated.h"

UCLASS()
class ASCENTMAPSSYSTEM_API AAMSActorMarker : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAMSActorMarker();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ACF)
	TObjectPtr<class UAMSMapMarkerComponent> MarkerComp;

};
