// // Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <GameplayTagContainer.h>

#include "ADSDialogueFunctionLibrary.generated.h"


/**
 *
 */
UCLASS()
class ASCENTDIALOGUESYSTEM_API UADSDialogueFunctionLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ADS)
    static class UADSDialogueMasterComponent* GetLocalDialogueMaster(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ADS)
    static FText ReplaceDialogueVariablesInText(const UObject* WorldContextObject, const FText& inString);

    UFUNCTION(BlueprintPure, Category = ADS)
    static FString ExtractLastStringFromGameplayTag( const FGameplayTag& inTag);

     UFUNCTION(BlueprintPure, Category = ADS)
    static FString ExtractActorNameFromSoftRef( const TSoftObjectPtr<AActor>& actorSoftRef);
};
