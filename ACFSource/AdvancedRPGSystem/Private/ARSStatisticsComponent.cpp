// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ARSStatisticsComponent.h"
#include "ARSFunctionLibrary.h"
#include "ARSLevelingSystemDataAsset.h"
#include "ARSTypes.h"
#include "Net/UnrealNetwork.h"
#include <Curves/CurveFloat.h>
#include <Engine/World.h>
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetSystemLibrary.h>
#include <TimerManager.h>

// Sets default values for this component's properties
UARSStatisticsComponent::UARSStatisticsComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked
    // every frame.  You can turn these features off to improve performance if you
    // don't need them.
    PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);
    // ...
    CharacterLevel = 1;
}

void UARSStatisticsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UARSStatisticsComponent, AttributeSet);
    DOREPLIFETIME(UARSStatisticsComponent, CurrentExps);
    DOREPLIFETIME(UARSStatisticsComponent, ExpToNextLevel);
    DOREPLIFETIME(UARSStatisticsComponent, Perks);
    DOREPLIFETIME(UARSStatisticsComponent, baseAttributeSet);
}

void UARSStatisticsComponent::InitializeAttributeSet()
{
    if (GetOwner()->HasAuthority())
    {
        InitilizeLevelData();

        Internal_InitializeStats();

        StartRegeneration();
    }
}

// Called when the game starts
void UARSStatisticsComponent::BeginPlay()
{

    Super::BeginPlay();

    if (bAutoInitialize)
    {
        InitializeAttributeSet();
    }
}

// Called every frame
void UARSStatisticsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UARSStatisticsComponent::RegenerateStat()
{
    for (const auto& elem : AttributeSet.Statistics)
    {
        if (elem.HasRegeneration)
        {
            if (regenDelay.Contains(elem.StatType))
            {
                FDateTime* before = regenDelay.Find(elem.StatType);
                if (before)
                {
                    const FTimespan delay = FDateTime::Now() - *before;
                    if (delay.GetSeconds() > elem.RegenDelay)
                    {
                        regenDelay.Remove(elem.StatType);
                    } else if (elem.RegenValue != 0.f)
                    {
                        continue;
                    }
                }
            }
            FStatisticValue modifier;
            modifier.Statistic = elem.StatType;
            modifier.Value = elem.RegenValue * RegenerationTimeInterval;
            Internal_ModifyStat(modifier, false);
        }
    }
}

void UARSStatisticsComponent::AddAttributeSetModifier_Implementation(const FAttributesSetModifier& attModifier)
{

    if (attModifier.StatisticsMod.Num() == 0 && attModifier.PrimaryAttributesMod.Num() == 0 && attModifier.AttributesMod.Num() == 0)
    {
        return;
    }

    if (!bIsInitialized)
    {
        storedUnactiveModifiers.Add(attModifier);
        return;
    }

    const FAttributesSetModifier convertedMod = CreateAdditiveAttributeSetModifireFromPercentage(attModifier);
    Internal_AddModifier(convertedMod);
}

void UARSStatisticsComponent::Internal_AddModifier(const FAttributesSetModifier& attModifier)
{

    activeModifiers.AddUnique(attModifier);

    GenerateStats();
}

void UARSStatisticsComponent::GenerateStats()
{
    // 1. Store old stat values for current/max adjustment later
    TArray<FStatistic> currentValuesCopy;
    for (const FStatistic& stat : AttributeSet.Statistics)
    {
        currentValuesCopy.Add(stat);
    }

    // 2. Reset base attributes (primary), parameters (secondary), and stats
    AttributeSet.Attributes = baseAttributeSet.Attributes;
    AttributeSet.Parameters = baseAttributeSet.Parameters;
    AttributeSet.Statistics = baseAttributeSet.Statistics;

    // 3. Apply primary attribute mods (multiplicative & additive)
    for (const FAttributesSetModifier& attModifier : activeModifiers)
    {
        for (const auto& mod : attModifier.PrimaryAttributesMod)
        {
            FAttribute* attr = AttributeSet.Attributes.FindByKey(mod.AttributeType);
            if (attr)
            {
                if (mod.ModType == EModifierType::EMultiplicative)
                    attr->Value *= mod.Value;
                else if (mod.ModType == EModifierType::EAdditive)
                    attr->Value += mod.Value;
            }
        }
    }

    // 4. Now generate secondary stats/statistics based on updated primary attributes
    GenerateSecondaryStat();

    // 5. Apply secondary attribute mods (multiplicative & additive)
    for (const FAttributesSetModifier& attModifier : activeModifiers)
    {
        for (const auto& mod : attModifier.AttributesMod)
        {
            FAttribute* param = AttributeSet.Parameters.FindByKey(mod.AttributeType);
            if (param)
            {
                if (mod.ModType == EModifierType::EMultiplicative)
                    param->Value *= mod.Value;
                else if (mod.ModType == EModifierType::EAdditive)
                    param->Value += mod.Value;
            }
        }
    }

    // 6. Apply statistics mods (multiplicative & additive)
    for (FStatistic& stat : AttributeSet.Statistics)
    {
        float MaxMult = 1.0f;
        float RegenMult = 1.0f;
        float MaxAdd = 0.f;
        float RegenAdd = 0.f;
        for (const FAttributesSetModifier& attModifier : activeModifiers)
        {
            for (const auto& mod : attModifier.StatisticsMod)
            {
                if (mod.AttributeType == stat.StatType)
                {
                    if (mod.ModType == EModifierType::EMultiplicative)
                    {
                        MaxMult *= mod.MaxValue;
                        RegenMult *= mod.RegenValue;
                    }
                    else if (mod.ModType == EModifierType::EAdditive)
                    {
                        MaxAdd += mod.MaxValue;
                        RegenAdd += mod.RegenValue;
                    }
                }
            }
        }
        stat.MaxValue = stat.MaxValue * MaxMult + MaxAdd;
        stat.RegenValue = stat.RegenValue * RegenMult + RegenAdd;
        stat.CurrentValue = FMath::Min(stat.CurrentValue, stat.MaxValue);
    }

    // 7. Restore current values proportionally if max changed
    for (FStatistic& stat : AttributeSet.Statistics)
    {
        FStatistic* oldStat = currentValuesCopy.FindByKey(stat);
        if (oldStat)
        {
            stat.CurrentValue = UARSFunctionLibrary::GetNewCurrentValueForNewMaxValue(oldStat->CurrentValue, oldStat->MaxValue, stat.MaxValue);
        }
    }

    AttributeSet.Sort();
    OnAttributeSetModified.Broadcast();
}


void UARSStatisticsComponent::Internal_ModifyStat(const FStatisticValue& StatMod, bool bResetDelay)
{
    if (!bIsInitialized)
    {
        return;
    }

    FStatistic* stat = AttributeSet.Statistics.FindByKey(StatMod.Statistic);

    if (stat)
    {
        const float oldValue = stat->CurrentValue;
        stat->CurrentValue += StatMod.Value; // *GetConsumptionMultiplierByStatistic(stat->StatType);

        if (stat->bClampToZero)
        {
            stat->CurrentValue = FMath::Clamp(stat->CurrentValue, 0.f, stat->MaxValue);
        } else
        {
            stat->CurrentValue = FMath::Clamp(stat->CurrentValue, -BIG_NUMBER, stat->MaxValue);
        }

        if (bResetDelay && stat->HasRegeneration && stat->RegenDelay > 0.f)
        {
            regenDelay.Add(stat->StatType, FDateTime::Now());
        }
        // AttributeSet.Sort();
        if (oldValue != stat->CurrentValue)
        {
            OnAttributeSetModified.Broadcast();
            OnStatisticChanged.Broadcast(stat->StatType, oldValue, stat->CurrentValue);
            if (FMath::IsNearlyZero(stat->CurrentValue))
            {
                OnStatisiticReachesZero.Broadcast(stat->StatType);
            }
        }
    }
}

void UARSStatisticsComponent::CalcualtePrimaryStats()
{
    AttributeSet.Attributes = baseAttributeSet.Attributes;

    for (const FAttributesSetModifier& attModifier : activeModifiers)
    {

        for (const auto& att : attModifier.PrimaryAttributesMod)
        {
            ensure(UARSFunctionLibrary::IsValidAttributeTag(att.AttributeType));
            if (UARSFunctionLibrary::IsValidAttributeTag(att.AttributeType))
            {
                FAttribute* _originalatt = AttributeSet.Attributes.FindByKey(att);
                if (_originalatt)
                {
                    *(_originalatt) = *(_originalatt) + att;
                } else
                {
                    AttributeSet.Attributes.Add(FAttribute(att.AttributeType, att.Value));
                }
            }
        }
    }
}

void UARSStatisticsComponent::CalcualteSecondaryStats()
{
    for (const FAttributesSetModifier& attModifier : activeModifiers)
    {
        for (const auto& att : attModifier.AttributesMod)
        {

            ensure(UARSFunctionLibrary::IsValidParameterTag(att.AttributeType));
            if (UARSFunctionLibrary::IsValidParameterTag(att.AttributeType))
            {
                FAttribute* _originalatt = AttributeSet.Parameters.FindByKey(att);
                if (_originalatt)
                {
                    *(_originalatt) = *(_originalatt) + att;
                } else
                {
                    AttributeSet.Parameters.Add(FAttribute(att.AttributeType, att.Value));
                }
            }
        }

        for (const auto& att : attModifier.StatisticsMod)
        {
            ensure(UARSFunctionLibrary::IsValidStatisticTag(att.AttributeType));
            if (UARSFunctionLibrary::IsValidStatisticTag(att.AttributeType))
            {
                FStatistic* _originalatt = AttributeSet.Statistics.FindByKey(att);
                if (_originalatt)
                {
                    *(_originalatt) = *(_originalatt) + att;
                } else
                {

                    AttributeSet.Statistics.Add(FStatistic(att.AttributeType, att.MaxValue, att.RegenValue));
                }
            }
        }
    }
}

FAttributesSetModifier UARSStatisticsComponent::CreateAdditiveAttributeSetModifireFromPercentage(const FAttributesSetModifier& attModifier)
{
    FAttributesSetModifier newatt;
    newatt.Guid = attModifier.Guid;

    for (const auto& att : attModifier.PrimaryAttributesMod)
    {
        if (att.ModType == EModifierType::EPercentage)
        {
            const FAttribute* originalatt = AttributeSet.Attributes.FindByKey(att);
            if (originalatt)
            {
                const float newval = originalatt->Value * att.Value / 100.f;
                const FAttributeModifier newMod(att.AttributeType, EModifierType::EAdditive, newval);
                newatt.PrimaryAttributesMod.AddUnique(newMod);
            }
        } else if (att.ModType == EModifierType::EAdditive)
        {
            const FAttributeModifier newMod(att.AttributeType, EModifierType::EAdditive, att.Value);
            newatt.PrimaryAttributesMod.AddUnique(newMod);
        }
    }
    for (const auto& att : attModifier.AttributesMod)
    {
        if (att.ModType == EModifierType::EPercentage)
        {
            const FAttribute* originalatt = AttributeSet.Parameters.FindByKey(att);
            if (originalatt)
            {
                const float newval = originalatt->Value * att.Value / 100.f;
                const FAttributeModifier newMod(att.AttributeType, EModifierType::EAdditive, newval);
                newatt.AttributesMod.AddUnique(newMod);
            }
        } else if (att.ModType == EModifierType::EAdditive)
        {
            const FAttributeModifier newMod(att.AttributeType, EModifierType::EAdditive, att.Value);
            newatt.AttributesMod.AddUnique(newMod);
        }
    }
    for (const auto& stat : attModifier.StatisticsMod)
    {
        const FStatistic* originalatt = AttributeSet.Statistics.FindByKey(stat);
        if (stat.ModType == EModifierType::EPercentage)
        {
            if (originalatt)
            {

                const float newval = originalatt->MaxValue * stat.MaxValue / 100.f;
                const float newregenval = originalatt->RegenValue * stat.RegenValue / 100.f;
                const FStatisticsModifier newMod(stat.AttributeType, EModifierType::EAdditive, newval, newregenval);
                newatt.StatisticsMod.AddUnique(newMod);
            }
        } else if (stat.ModType == EModifierType::EAdditive)
        {
            const FStatisticsModifier newMod(stat.AttributeType, EModifierType::EAdditive, stat.MaxValue, stat.RegenValue);
            newatt.StatisticsMod.AddUnique(newMod);
        }
    }
    return newatt;
}

void UARSStatisticsComponent::GenerateSecondaryStat()
{
    AttributeSet.Parameters = DefaultAttributeSet.Parameters;
    AttributeSet.Statistics = DefaultAttributeSet.Statistics;

    if (StatsLoadMethod != EStatsLoadMethod::EUseDefaultsWithoutGeneration)
    {
        GenerateSecondaryStatFromCurrentPrimaryStat();
        //         baseAttributeSet.Parameters = AttributeSet.Parameters;
        //         baseAttributeSet.Statistics = AttributeSet.Statistics;
    }
    CalcualteSecondaryStats();
}

void UARSStatisticsComponent::GenerateSecondaryStatFromCurrentPrimaryStat()
{

    for (const FAttribute& primaryatt : AttributeSet.Attributes)
    {
        FGenerationRule rules;

        if (!UARSFunctionLibrary::TryGetGenerationRuleByPrimaryAttributeType(primaryatt.AttributeType, rules))
        {

            return;
        }

        for (const FAttributeInfluence& att : rules.InfluencedParameters)
        {
            if (att.CurveValue)
            {
                FAttribute* targetAttribute = AttributeSet.Parameters.FindByKey(att.TargetParameter);
                if (targetAttribute)
                {
                    targetAttribute->Value += att.CurveValue->GetFloatValue(primaryatt.Value);
                } else
                {
                    const float param = att.CurveValue->GetFloatValue(primaryatt.Value);
                    const FAttribute localatt = FAttribute(att.TargetParameter, param);
                    AttributeSet.Parameters.AddUnique(localatt);
                }
            }
        }

        for (const FStatInfluence& stat : rules.InfluencedStatistics)
        {

            if (stat.CurveMaxValue)
            {
                FStatistic* targetStat = AttributeSet.Statistics.FindByKey(stat.TargetStat);
                if (targetStat)
                {
                    targetStat->MaxValue += stat.CurveMaxValue->GetFloatValue(primaryatt.Value);
                    targetStat->CurrentValue = targetStat->bStartFromZero ? 0.f : targetStat->MaxValue;
                } else
                {
                    const float param = stat.CurveMaxValue->GetFloatValue(primaryatt.Value);
                    const FStatistic localstat = FStatistic(stat.TargetStat, param, 0.f);
                    AttributeSet.Statistics.AddUnique(localstat);
                }
            }
            FStatistic* targetStat = AttributeSet.Statistics.FindByKey(stat.TargetStat);
            if (targetStat && stat.CurveRegenValue)
            {
                targetStat->RegenValue += stat.CurveRegenValue->GetFloatValue(primaryatt.Value);
                targetStat->HasRegeneration = targetStat->RegenValue != 0.f;
            }
        }
    }
}

void UARSStatisticsComponent::StartRegeneration_Implementation()
{
    if (!bIsRegenerationStarted && bCanRegenerateStatistics)
    {
        UWorld* world = GetWorld();
        if (world)
        {
            world->GetTimerManager().SetTimer(RegenTimer, this, &UARSStatisticsComponent::RegenerateStat, RegenerationTimeInterval, true);
            bIsRegenerationStarted = true;
        }
    }
}

void UARSStatisticsComponent::StopRegeneration_Implementation()
{
    if (bIsRegenerationStarted && RegenTimer.IsValid())
    {
        UWorld* world = GetWorld();
        // Calling MyUsefulFunction after 5 seconds without looping
        world->GetTimerManager().ClearTimer(RegenTimer);
        bIsRegenerationStarted = false;
    }
}

void UARSStatisticsComponent::AddExp_Implementation(int32 exp)
{
    if (LevelingType == ELevelingType::ECantLevelUp)
    {
        UE_LOG(LogTemp, Warning, TEXT("This Character can not level up! ARSStatisticsComponent"));
        return;
    }

    Internal_AddExp(exp);

    OnCurrentExpValueChanged.Broadcast(CurrentExps, exp);
}


void UARSStatisticsComponent::Internal_AddExp(int32 exp)
{
    CurrentExps += exp;

    if (CurrentExps >= ExpToNextLevel && CharacterLevel < UARSFunctionLibrary::GetMaxLevel())
    {
        const int32 remainingExps = CurrentExps - ExpToNextLevel;
        CurrentExps = 0;
        CharacterLevel++;
        InitilizeLevelData();

        switch (LevelingType)
        {
        case ELevelingType::EGenerateNewStatsFromCurves:
            Internal_InitializeStats();
            break;
        case ELevelingType::EAssignPerksManually:
            Perks += PerksObtainedOnLevelUp;
            break;
        default:
            UE_LOG(LogTemp, Error, TEXT("A character that cannot level, just leveled! ARSStatisticsComponent"));
            break;
        }
        OnLevelUp(CharacterLevel, remainingExps);
        Internal_AddExp(remainingExps);
    }
}

void UARSStatisticsComponent::RemoveAttributeSetModifier_Implementation(const FAttributesSetModifier& attModifier)
{
    FAttributesSetModifier* localmod = activeModifiers.FindByKey(attModifier);
    if (localmod)
    {

        activeModifiers.RemoveSingle(*(localmod));

        GenerateStats();
    }
}

void UARSStatisticsComponent::AddStatisticConsumptionMultiplier_Implementation(FGameplayTag statisticTag, float multiplier /*= 1.0f*/)
{
    if (UARSFunctionLibrary::IsValidStatisticTag(statisticTag))
    {
        StatisticConsumptionMultiplier.Add(statisticTag, multiplier);
    }
}

float UARSStatisticsComponent::GetConsumptionMultiplierByStatistic(FGameplayTag statisticTag) const
{
    if (UARSFunctionLibrary::IsValidStatisticTag(statisticTag))
    {
        const float* _mult = StatisticConsumptionMultiplier.Find(statisticTag);
        if (_mult)
        {
            return *(_mult);
        }
    }
    return 1.0f;
}

bool UARSStatisticsComponent::CheckCosts(const TArray<FStatisticValue>& Costs) const
{
    for (const FStatisticValue& cost : Costs)
    {
        if (!CheckCost(cost))
        {
            return false;
        }
    }
    return true;
}

bool UARSStatisticsComponent::CheckPrimaryAttributesRequirements(const TArray<FAttribute>& Requirements) const
{
    for (const FAttribute& att : Requirements)
    {
        if (!UARSFunctionLibrary::IsValidAttributeTag(att.AttributeType))
        {
            UE_LOG(LogTemp, Log,
                TEXT("Invalid Primary Attribute Tag!!! - "
                    "CheckPrimaryAttributeRequirements"));
            return false;
        }
        const FAttribute* localatt = AttributeSet.Attributes.FindByKey(att.AttributeType);
        if (localatt && localatt->Value < att.Value)
        {
            return false;
        }
    }
    return true;
}

bool UARSStatisticsComponent::CheckCost(const FStatisticValue& Cost) const
{
    const FStatistic* stat = AttributeSet.Statistics.FindByKey(Cost.Statistic);
    if (stat)
    {
        return stat->CurrentValue > (Cost.Value * GetConsumptionMultiplierByStatistic(stat->StatType));
    }
    UE_LOG(LogTemp, Warning, TEXT("Missing Statistic! - ARSStatistic Component"));
    return false;
}

void UARSStatisticsComponent::ConsumeStatistics(const TArray<FStatisticValue>& Costs)
{
    for (const FStatisticValue& cost : Costs)
    {
        FStatisticValue modifier = cost;
        modifier.Value *= -1;
        ModifyStat(modifier);
    }
}

void UARSStatisticsComponent::OnRep_AttributeSet()
{
    OnAttributeSetModified.Broadcast();
}

void UARSStatisticsComponent::Internal_InitializeStats()
{
    bIsInitialized = false;

    AttributeSet.Statistics.Empty();
    AttributeSet.Attributes.Empty();
    AttributeSet.Parameters.Empty();

    TArray<FStatistic> currentValues;
    switch (StatsLoadMethod)
    {
    case EStatsLoadMethod::EUseDefaultsWithoutGeneration:
        baseAttributeSet = DefaultAttributeSet;
        AttributeSet = baseAttributeSet;
        break;
    case EStatsLoadMethod::EGenerateFromDefaultsPrimary:
        baseAttributeSet = DefaultAttributeSet;
        break;
    case EStatsLoadMethod::ELoadByLevel:
        baseAttributeSet.Attributes = GetPrimitiveAttributesForCurrentLevel();
        AttributeSet = baseAttributeSet;
        break;
    default:
        break;
    }

    if (StatsLoadMethod != EStatsLoadMethod::EUseDefaultsWithoutGeneration)
    {
        GenerateStats();
    }

    for (auto& statistic : AttributeSet.Statistics)
    {
        statistic.CurrentValue = statistic.bStartFromZero ? 0.f : statistic.MaxValue;
    }

    bIsInitialized = true;

    for (const FAttributesSetModifier& modifier : storedUnactiveModifiers)
    {
        AddAttributeSetModifier(modifier);
    }
    storedUnactiveModifiers.Empty();
}

void UARSStatisticsComponent::OnLevelUp_Implementation(int32 newlevel, int32 _remainingExp)
{
    CharacterLevel = newlevel;

    OnCharacterLevelUp.Broadcast(CharacterLevel);
}

void UARSStatisticsComponent::ModifyStatistic(FGameplayTag Stat, float value)
{
    FStatisticValue mod = FStatisticValue(Stat, value);

    ModifyStat(mod);
}

void UARSStatisticsComponent::RefillStat(FGameplayTag Stat)
{
    if (UARSFunctionLibrary::IsValidStatisticTag(Stat))
    {
        ModifyStatistic(Stat, GetMaxValueForStatitstic(Stat));
    }
}

float UARSStatisticsComponent::GetCurrentValueForStatitstic(FGameplayTag stat) const
{

    if (!UARSFunctionLibrary::IsValidStatisticTag(stat))
    {
        UE_LOG(LogTemp, Warning, TEXT("INVALID STATISTIC TAG -  -  ARSStatistic Component"));
        return 0.f;
    }

    const FStatistic* intStat = AttributeSet.Statistics.FindByKey(stat);

    if (intStat)
    {
        return intStat->CurrentValue;
    }
    return 0.f;
}

float UARSStatisticsComponent::GetMaxValueForStatitstic(FGameplayTag stat) const
{
    if (!UARSFunctionLibrary::IsValidStatisticTag(stat))
    {
        UE_LOG(LogTemp, Warning, TEXT("INVALID STATISTIC TAG -  -  ARSStatistic Component"));
        return 0.f;
    }

    const FStatistic* intStat = AttributeSet.Statistics.FindByKey(stat);

    if (intStat)
    {
        return intStat->MaxValue;
    }
    return 0.f;
}

float UARSStatisticsComponent::GetNormalizedValueForStatitstic(FGameplayTag statTag) const
{
    const float max = GetMaxValueForStatitstic(statTag);
    const float value = GetCurrentValueForStatitstic(statTag);

    if (max != 0.f)
    {
        return value / max;
    }
    //     UE_LOG(LogTemp, Warning,
    //         TEXT("Missing Statistic! -  -  ARSStatistic Component"));
    return 0.f;
}

float UARSStatisticsComponent::GetCurrentPrimaryAttributeValue(FGameplayTag attributeTag) const
{
    if (!UARSFunctionLibrary::IsValidAttributeTag(attributeTag))
    {
        UE_LOG(LogTemp, Warning, TEXT("INVALID PRIMARY ATTRIBUTE TAG -  -  ARSStatistic Component"));
        return 0.f;
    }

    const FAttribute* intStat = AttributeSet.Attributes.FindByKey(attributeTag);

    if (intStat)
    {
        return intStat->Value;
    }

    UE_LOG(LogTemp, Warning, TEXT("Missing  Primary Attribute '%s'! -  -  ARSStatistic Component"), *attributeTag.GetTagName().ToString());

    return 0.f;
}

float UARSStatisticsComponent::GetCurrentAttributeValue(FGameplayTag attributeTag) const
{
    if (!UARSFunctionLibrary::IsValidParameterTag(attributeTag))
    {
        UE_LOG(LogTemp, Warning, TEXT("INVALID SECONDARY ATTRIBUTE TAG -  -  ARSStatistic Component"));
        return 0.f;
    }

    const FAttribute* intStat = AttributeSet.Parameters.FindByKey(attributeTag);

    if (intStat)
    {
        return intStat->Value;
    }

    UE_LOG(LogTemp, Warning, TEXT("Missing  Secondary Attribute '%s! - ARSStatistic Component"), *attributeTag.GetTagName().ToString());

    return 0.f;
}

FAttributesSet UARSStatisticsComponent::GetCurrentAttributeSet() const
{
    return AttributeSet;
}

float UARSStatisticsComponent::GetBaseAttributeValue(FGameplayTag attributeTag) const
{
    // First, check primary attributes
    const FAttribute* primary = baseAttributeSet.Attributes.FindByKey(attributeTag);
    if (primary)
        return primary->Value;

    // Next, check secondary attributes
    const FAttribute* secondary = baseAttributeSet.Parameters.FindByKey(attributeTag);
    if (secondary)
        return secondary->Value;

    // Finally, check statistics
    const FStatistic* stat = baseAttributeSet.Statistics.FindByKey(attributeTag);
    if (stat)
        return stat->MaxValue;

    // Not found
    return 0.f;
}

int32 UARSStatisticsComponent::GetExpOnDeath() const
{
    if (!CanLevelUp())
    {
        return ExpToGiveOnDeath;
    }

    if (ExpToGiveOnDeathByCurrentLevel)
    {
        float expToGive = ExpToGiveOnDeathByCurrentLevel->GetFloatValue(CharacterLevel);
        return FMath::TruncToInt(expToGive);
    }

    UE_LOG(LogTemp, Error, TEXT("Invalid ExpToGiveOnDeathByCurrentLevel Curve!"));
    return -1;
}

void UARSStatisticsComponent::AssignPerkToPrimaryAttribute_Implementation(FGameplayTag attributeTag, int32 numPerks /*= 1*/)
{
    if (numPerks > Perks)
    {
        UE_LOG(LogTemp, Warning, TEXT("You don't have enough perks!"));
        return;
    }

    PermanentlyModifyPrimaryAttribute(attributeTag, numPerks);
    Perks -= numPerks;
}

void UARSStatisticsComponent::PermanentlyModifyPrimaryAttribute_Implementation(FGameplayTag attribute, float deltaValue /*= 1.0f*/)
{
    const FAttribute* currValue = DefaultAttributeSet.Attributes.FindByKey(attribute);
    if (currValue)
    {
        FAttribute newValue(currValue->AttributeType, currValue->Value + deltaValue);
        DefaultAttributeSet.Attributes.Remove(newValue);
        DefaultAttributeSet.Attributes.AddUnique(newValue);
        InitializeAttributeSet();
    }
}

void UARSStatisticsComponent::InitilizeLevelData()
{
    ExpToNextLevel = GetTotalExpsForLevel(CharacterLevel);
}

int32 UARSStatisticsComponent::GetTotalExpsForLevel(int32 level) const
{
    if (ExpForNextLevelCurve)
    {
        const float nextlevelexp = ExpForNextLevelCurve->GetFloatValue(level);
        return FMath::TruncToInt(nextlevelexp);
    }
    return -1;
}

int32 UARSStatisticsComponent::GetTotalExpsAcquired() const
{
    return GetExpsForLevel(CharacterLevel - 1) + GetCurrentExp();
}

int32 UARSStatisticsComponent::GetExpsForLevel(int32 level) const
{
    if (level > 1)
    {
        return GetTotalExpsForLevel(level) - GetTotalExpsForLevel(level - 1);
    }
    return GetTotalExpsForLevel(level);
}

void UARSStatisticsComponent::ModifyStat_Implementation(const FStatisticValue& StatMod)
{
    Internal_ModifyStat(StatMod);
}

TArray<FAttribute> UARSStatisticsComponent::GetPrimitiveAttributesForCurrentLevel()
{
    return Internal_GetPrimitiveAttributesForCurrentLevel();
}

void UARSStatisticsComponent::OnComponentLoaded_Implementation()
{
    if (StatsLoadMethod != EStatsLoadMethod::EUseDefaultsWithoutGeneration)
    {
        GenerateStats();
    }
}

void UARSStatisticsComponent::OnComponentSaved_Implementation() {}

TArray<FAttribute> UARSStatisticsComponent::Internal_GetPrimitiveAttributesForCurrentLevel()
{
    TArray<FAttribute> attributes;

    if (AttributesByLevelConfig)
    {
        AttributesByLevelConfig->GetAllAttributesValueByLevel(CharacterLevel, attributes);
    }

    return attributes;
}

void UARSStatisticsComponent::AddTimedAttributeSetModifier_Implementation(const FAttributesSetModifier& attModifier, float duration)
{
    if (duration == 0.f)
    {
        return;
    }

    if (!attModifier.AttributesMod.IsValidIndex(0) && !attModifier.PrimaryAttributesMod.IsValidIndex(0) && !attModifier.StatisticsMod.IsValidIndex(0))
    {
        return;
    }

    Internal_AddModifier(attModifier);

    FTimerDelegate TimerDel;
    FTimerHandle TimerHandle;
    TimerDel.BindUFunction(this, FName("RemoveAttributeSetModifier"), attModifier);

    UWorld* world = GetWorld();
    if (world)
    {
        world->GetTimerManager().SetTimer(TimerHandle, TimerDel, duration, false);
    }
}

void UARSStatisticsComponent::ReinitializeAttributeSetFromNewDefault_Implementation(const FAttributesSet& newDefault)
{
    DefaultAttributeSet = newDefault;

    InitializeAttributeSet();
}

void UARSStatisticsComponent::SetNewLevelAndReinitialize_Implementation(int32 newLevel)
{
    CharacterLevel = newLevel;

    InitializeAttributeSet();
}