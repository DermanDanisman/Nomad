// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFDamageType.h"
#include "ACFTypes.h"
#include "ARSTypes.h"
#include "CoreMinimal.h"
#include "Items/ACFItem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <Engine/DataTable.h>
#include <GameplayTagContainer.h>

#include "ACFFunctionLibrary.generated.h"

class AACFCharacter;
struct FAttributesSet;
struct FStartingItem;
class UARSStatisticsComponent;
class AActor;

UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFFunctionLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static EACFDirection GetHitDirectionByHitResult(const AActor* hitDealer, const FHitResult& hitResult);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static FVector GetActorsRelativeDirectionVector(const FACFDamageEvent& damageEvent);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static EACFDirection GetActorsRelativeDirection(const AActor* hitDealer, const AActor* receiver);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static EACFDirection GetDirectionFromInput(const AActor* actor, const FVector& direction);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static FVector GetActorsOppositeRelativeDirection(const FACFDamageEvent& damageEvent);

    UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static FGameplayTag GetDefaultDeathState();

    UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static FGameplayTag GetDefaultHitState();

    UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static FGameplayTag GetHealthTag();

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float ReduceDamageByPercentage(float Damage, float Percentage);

    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static void PlayImpactEffect(const FImpactEffect& effect, const FVector& impactLocation, class AActor* instigator, const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static void PlayActionEffect(const FActionEffect& effect, class ACharacter* instigator, const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static void PlayActionEffectLocally(const FActionEffect& effect, class ACharacter* instigator, const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static void PlayFootstepEffect(class APawn* instigator, FName footBone, const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static EBattleType GetBattleType(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static bool AreEnemyTeams(const UObject* WorldContextObject, ETeam teamA, ETeam teamB);

    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static bool TryGetTeamsConfig(const UObject* WorldContextObject, TMap<ETeam, FTeamInfo>& outConfig);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float CalculateDistanceBetweenActors(const AActor* characterA, const AActor* characterB);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static FVector GetPointAtDirectionAndDistanceFromActor(const AActor* targetActor, const FVector& direction, float distance, bool bShowDebug = false);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float CalculateDistanceBetweenCharactersExtents(const ACharacter* characterA, const ACharacter* characterB);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float GetCharacterExtent(const ACharacter* characterA);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float GetCharacterDistantToPointConsideringExtent(const ACharacter* characterA, const FVector& point);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static float CalculateAngleBetweenVectors(const FVector& vectorA, const FVector& vectorB);

    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class AACFHUD* GetACFHUD(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class AACFPlayerController* GetLocalACFPlayerController(const UObject* WorldContextObject);

    /*Works on server only*/
    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class AACFGameMode* GetACFGameMode(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class AACFGameState* GetACFGameState(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class AACFCharacter* GetLocalACFPlayerCharacter(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static class UACFTeamManagerComponent* GetACFTeamManager(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static bool ShouldExecuteAction(const FActionChances& action, const AACFCharacter* characterOwner);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static bool GetCharacterAttributes(const TSubclassOf<class AACFCharacter>& characterClass, FAttributesSet& outData, bool bInitialize = true);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static UARSStatisticsComponent* GetCharacterStatisticComp(const TSubclassOf<class AACFCharacter>& characterClass, bool bInitialize = true);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static AACFCharacter* GetCharacterDefaults(const TSubclassOf<class AACFCharacter>& characterClass);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static bool GetCharacterStartingItems(const TSubclassOf<class AACFCharacter>& characterClass, TArray<FStartingItem>& outItems);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static bool GetCharacterName(const TSubclassOf<class AACFCharacter>& characterClass, FText& outName);

    UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"), Category = ACFLibrary)
    static AActor* FindNearestActorOfClass(const UObject* WorldContextObject, TSubclassOf<class AActor> actorToFind, AActor* origin);

    UFUNCTION(BlueprintCallable, Category = ACFLibrary)
    static int32 ExtractIndexWithProbability(const TArray<float>& weights);

    //** BUILD **//
    //     UFUNCTION(BlueprintPure, Category = ACFLibrary)
    //     static EBuildType GetBuildType();

    UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static bool IsShippingBuild();

    UFUNCTION(BlueprintPure, Category = ACFLibrary)
    static bool IsEditor();
    //     template <typename TEnum>
    //     static FORCEINLINE FString GetEnumValueAsString(const FString& Name, TEnum Value)
    //     {
    //         const UEnum* enumPtr = FindObject<UEnum>(ANY_PACKAGE, *Name, true);
    //         if (!enumPtr) {
    //             return FString("Invalid");
    //         }
    //
    //         return enumPtr->GetNameByValue((int64)Value).ToString();
    //     }
};