// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/StatusEffect/Utility/NomadStatusEffectUtils.h"
#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "ARSStatisticsComponent.h"

UNomadStatusEffectConfigBase* UNomadStatusEffectUtils::FindConfigByTag(const TArray<UNomadStatusEffectConfigBase*>& Configs, FGameplayTag Tag)
{
    for (UNomadStatusEffectConfigBase* Config : Configs)
    {
        if (Config && Config->EffectTag == Tag)
        {
            return Config;
        }
    }
    return nullptr;
}

void UNomadStatusEffectUtils::ApplyStatModifications(UARSStatisticsComponent* StatsComp, const TArray<FStatisticValue>& Modifications)
{
    if (!StatsComp) return;
    for (const FStatisticValue& Mod : Modifications)
    {
        StatsComp->ModifyStat(Mod);
    }
}