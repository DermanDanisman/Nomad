// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include <GameplayTagContainer.h>

#include "ADSDialogueSubsystem.generated.h"

/**
 *
 */
class UADSDialoguePartecipantComponent;

UCLASS()
class ASCENTDIALOGUESYSTEM_API UADSDialogueSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    void RegisterParticipant(UADSDialoguePartecipantComponent* participant);

    void UnregisterParticipant(const FGameplayTag& participant);

    /*Returns the dialogue component of the provided participant*/
    UFUNCTION(BlueprintCallable, Category = ADS)
    UADSDialoguePartecipantComponent* FindParticipant(const FGameplayTag& participant) const;

    UFUNCTION(BlueprintCallable, Category = ADS)
    FGameplayTag GetDefaultPlayerResponseTag() const;

private:
    TMap<FGameplayTag, UADSDialoguePartecipantComponent*> Participants;
};
