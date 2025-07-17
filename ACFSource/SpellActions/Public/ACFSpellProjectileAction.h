// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Actions/ACFBaseAction.h"
#include "Actions/ACFComboAction.h"
#include "CoreMinimal.h"

#include "ACFSpellProjectileAction.generated.h"


/**
 *
 */
UCLASS()
class SPELLACTIONS_API UACFSpellProjectileAction : public UACFComboAction {
    GENERATED_BODY()

public:
    UACFSpellProjectileAction();

protected:
    virtual void OnNotablePointReached_Implementation() override;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    TSubclassOf<class AACFProjectile> ProjectileClass;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    EShootDirection ShootDirection;

    /*The socket from which the projectile spells will be thrown, array index will match the combo counter*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    TArray<FName> LaunchSocketNames;

    UFUNCTION(BlueprintPure, Category = ACF)
    FName GetDesiredSocketName() const;

  

};
