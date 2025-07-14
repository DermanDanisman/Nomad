#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ARSTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NomadStatusEffectUtils.generated.h"

class UARSStatisticsComponent;
class UNomadStatusEffectConfigBase;

UCLASS()
class NOMADDEV_API UNomadStatusEffectUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Lookup config by tag from a list
    UFUNCTION(BlueprintCallable, Category="Status Effect|Utils")
    static UNomadStatusEffectConfigBase* FindConfigByTag(const TArray<UNomadStatusEffectConfigBase*>& Configs, FGameplayTag Tag);

    // Apply stat modifications safely
    UFUNCTION(BlueprintCallable, Category="Status Effect|Utils")
    static void ApplyStatModifications(UARSStatisticsComponent* StatsComp, const TArray<FStatisticValue>& Modifications);
};