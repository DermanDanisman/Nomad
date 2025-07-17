// // Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "ADSDialogueMasterComponent.generated.h"


UCLASS(ClassGroup = (ATS), meta = (BlueprintSpawnableComponent))
class ASCENTDIALOGUESYSTEM_API UADSDialogueMasterComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UADSDialogueMasterComponent();

    UFUNCTION(BlueprintCallable, Category = ADS)
    void AddDialogueVariable(FString key, FString value);

    UFUNCTION(BlueprintCallable, Category = ADS)
    void RemoveDialogueVariable(FString key);

    UFUNCTION(BlueprintPure, Category = ADS)
	bool GetDialogueVariable(FString key, FString& outValue) const;

    UFUNCTION(BlueprintCallable, Category = ADS)
    FText ReplaceVariablesInText(const FText& inText);


    UFUNCTION(Server, Reliable, Category = ADS)
    void MoveControlledPlayerToPosition(const FTransform& finalPoint);
    
    
 protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Savegame, Category = ADS)
    TMap<FString, FString> DialogueVariables;
};
