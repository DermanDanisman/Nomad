// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/NomadInfiniteStatusEffect.h"
#include "Core/Data/StatusEffect/NomadInfiniteEffectConfig.h"
#include "ARSStatisticsComponent.h"
#include "Core/Debug/NomadLogCategories.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "Core/StatusEffect/Utility/NomadStatusEffectUtils.h"
#include "Kismet/GameplayStatics.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadInfiniteStatusEffect::UNomadInfiniteStatusEffect()
{
    CachedTickInterval = 5.0f;
    bCachedHasPeriodicTick = false;
    StartTime = 0.0f;
    TickCount = 0;
    AppliedModifierGuid = FGuid();
    LastTickDamage = 0.0f;
}

// =====================================================
//         STACKING / REFRESH LOGIC (BP & C++)
// =====================================================

void UNomadInfiniteStatusEffect::OnStacked_Implementation(const int32 NewStackCount)
{
    StackCount = NewStackCount;
    RemoveAttributeSetModifier();
    ApplyAttributeSetModifier();
}

void UNomadInfiniteStatusEffect::OnRefreshed_Implementation()
{
    RemoveAttributeSetModifier();
    ApplyAttributeSetModifier();
}

void UNomadInfiniteStatusEffect::OnUnstacked(int32 NewStackCount)
{
    StackCount = NewStackCount;
    RemoveAttributeSetModifier();
    ApplyAttributeSetModifier();
}

// =====================================================
//         CONFIGURATION ACCESS & APPLICATION
// =====================================================

UNomadInfiniteEffectConfig* UNomadInfiniteStatusEffect::GetEffectConfig() const
{
    return Cast<UNomadInfiniteEffectConfig>(EffectConfig.IsNull() ? nullptr : EffectConfig.LoadSynchronous());
}

void UNomadInfiniteStatusEffect::ApplyConfiguration()
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (!Config) return;
    if (!Config->IsConfigValid()) return;

    CacheConfigurationValues();
    ApplyBaseConfiguration();
    ApplyConfigurationTag();
    ApplyConfigurationIcon();
}

bool UNomadInfiniteStatusEffect::HasValidConfiguration() const
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    return Config && Config->IsConfigValid();
}

void UNomadInfiniteStatusEffect::ApplyConfigurationTag()
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config && Config->EffectTag.IsValid())
    {
        SetStatusEffectTag(Config->EffectTag);
    }
}

void UNomadInfiniteStatusEffect::ApplyConfigurationIcon()
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config && !Config->Icon.IsNull())
    {
        UTexture2D* LoadedIcon = Config->Icon.LoadSynchronous();
        if (LoadedIcon)
        {
            SetStatusIcon(LoadedIcon);
        }
    }
}

FGameplayTag UNomadInfiniteStatusEffect::GetEffectiveTag() const
{
    return GetStatusEffectTag();
}

ENomadStatusCategory UNomadInfiniteStatusEffect::GetStatusCategory_Implementation() const
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config)
    {
        return Config->Category;
    }
    return Super::GetStatusCategory_Implementation();
}

float UNomadInfiniteStatusEffect::GetUptime() const
{
    if (!CharacterOwner || !CharacterOwner->GetWorld() || StartTime <= 0.0f)
        return 0.0f;
    return CharacterOwner->GetWorld()->GetTimeSeconds() - StartTime;
}

bool UNomadInfiniteStatusEffect::CanBeManuallyRemoved() const
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    return Config ? Config->bCanBeManuallyRemoved : false;
}

bool UNomadInfiniteStatusEffect::ShouldPersistThroughSaveLoad() const
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    return Config ? Config->bPersistThroughSaveLoad : false;
}

// =====================================================
//         MANUAL/FORCED CONTROL (REMOVAL)
// =====================================================

bool UNomadInfiniteStatusEffect::TryManualRemoval(AActor* Remover)
{
    if (!CanBeManuallyRemoved())
        return false;

    bool bAllowRemoval = OnManualRemovalAttempt_Implementation(Remover);
    if (!bAllowRemoval)
    {
        OnManualRemovalAttempt(Remover);
        if (!bAllowRemoval)
            return false;
    }
    if (CharacterOwner)
    {
        if (UNomadStatusEffectManagerComponent* Manager = CharacterOwner->FindComponentByClass<UNomadStatusEffectManagerComponent>())
        {
            Manager->Nomad_RemoveStatusEffect(GetEffectiveTag());
            return true;
        }
    }
    EndEffect();
    return true;
}

void UNomadInfiniteStatusEffect::ForceRemoval()
{
    EndEffect();
}

void UNomadInfiniteStatusEffect::Nomad_OnStatusEffectStarts(ACharacter* Character)
{
    Super::Nomad_OnStatusEffectStarts(Character);
    
    OnStatusEffectStarts_Implementation(Character);
}

void UNomadInfiniteStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    Super::OnStatusEffectStarts_Implementation(Character);
    SetEffectLifecycleState(EEffectLifecycleState::Active);

    if (Character && Character->GetWorld())
    {
        StartTime = Character->GetWorld()->GetTimeSeconds();
        TickCount = 0;

        UNomadInfiniteEffectConfig* Config = GetEffectConfig();
        if (Config && Config->OnActivationStatModifications.Num() > 0)
        {
            int32 CurrentStacks = GetCurrentStackCount();
            StackCount = CurrentStacks;
            TArray<FStatisticValue> ScaledMods = Config->OnActivationStatModifications;
            for (FStatisticValue& Mod : ScaledMods)
                Mod.Value *= CurrentStacks;
            ApplyHybridEffect(ScaledMods, Character, Config);
            OnStatModificationsApplied_Implementation(ScaledMods);
            OnStatModificationsApplied(ScaledMods);
        }

        ApplyAttributeSetModifier();
        SetupInfiniteTicking();

        OnInfiniteEffectActivated_Implementation(Character);
        OnInfiniteEffectActivated(Character);
    }
}

void UNomadInfiniteStatusEffect::OnStatusEffectEnds_Implementation()
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config && Config->OnDeactivationStatModifications.Num() > 0)
    {
        if (GetEffectLifecycleState() != EEffectLifecycleState::Active && GetEffectLifecycleState() != EEffectLifecycleState::Ending)
            return;

        ApplyHybridEffect(Config->OnDeactivationStatModifications, CharacterOwner, Config);
        OnStatModificationsApplied_Implementation(Config->OnDeactivationStatModifications);
        OnStatModificationsApplied(Config->OnDeactivationStatModifications);
    }

    RemoveAttributeSetModifier();
    ClearInfiniteTicking();
    OnInfiniteEffectDeactivated_Implementation();
    OnInfiniteEffectDeactivated();

    Super::OnStatusEffectEnds_Implementation();
}

void UNomadInfiniteStatusEffect::OnInfiniteEffectActivated_Implementation(ACharacter* Character)
{
}

void UNomadInfiniteStatusEffect::OnInfiniteTick_Implementation(float Uptime, int32 CurrentTickCount)
{
}

bool UNomadInfiniteStatusEffect::OnManualRemovalAttempt_Implementation(AActor* Remover)
{
    return CanBeManuallyRemoved();
}

void UNomadInfiniteStatusEffect::OnInfiniteEffectDeactivated_Implementation()
{
    ClearInfiniteTicking();
}

void UNomadInfiniteStatusEffect::OnStatModificationsApplied_Implementation(const TArray<FStatisticValue>& StatisticModifications)
{
}

void UNomadInfiniteStatusEffect::SetupInfiniteTicking()
{
    if (!bCachedHasPeriodicTick || CachedTickInterval <= 0.0f || !CharacterOwner || !CharacterOwner->GetWorld())
        return;

    UWorld* World = CharacterOwner->GetWorld();
    FTimerManager& TimerManager = World->GetTimerManager();

    TimerManager.SetTimer(
        TickTimerHandle,
        this,
        &UNomadInfiniteStatusEffect::HandleInfiniteTick,
        CachedTickInterval,
        true
    );
}

void UNomadInfiniteStatusEffect::ClearInfiniteTicking()
{
    if (!CharacterOwner || !CharacterOwner->GetWorld())
        return;

    UWorld* World = CharacterOwner->GetWorld();
    FTimerManager& TimerManager = World->GetTimerManager();

    TimerManager.ClearTimer(TickTimerHandle);
}

void UNomadInfiniteStatusEffect::HandleInfiniteTick()
{
    TickCount++;
    float CurrentUptime = GetUptime();

    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config && Config->OnTickStatModifications.Num() > 0)
    {
        int32 CurrentStacks = GetCurrentStackCount();
        StackCount = CurrentStacks;
        TArray<FStatisticValue> ScaledMods = Config->OnTickStatModifications;
        for (FStatisticValue& Mod : ScaledMods)
            Mod.Value *= CurrentStacks;
        ApplyHybridEffect(ScaledMods, CharacterOwner, Config);
        OnStatModificationsApplied_Implementation(ScaledMods);
        OnStatModificationsApplied(ScaledMods);
    }

    OnInfiniteTick_Implementation(CurrentUptime, TickCount);
    OnInfiniteTick(CurrentUptime, TickCount);
}

void UNomadInfiniteStatusEffect::ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfigObj)
{
    if (!IsValid(Target) || Target->IsPendingKillPending() || !EffectConfigObj)
        return;

    UNomadInfiniteEffectConfig* Config = Cast<UNomadInfiniteEffectConfig>(EffectConfigObj);
    if (!Config) return;

    float EffectDamage = 0.0f;
    AActor* Causer = GetSafeDamageCauser(Target);

    switch (Config->ApplicationMode)
    {
    case EStatusEffectApplicationMode::StatModification:
        {
            UARSStatisticsComponent* StatsComp = Target->FindComponentByClass<UARSStatisticsComponent>();
            if (StatsComp)
            {
                UNomadStatusEffectUtils::ApplyStatModifications(StatsComp, StatMods);
                for (const FStatisticValue& Mod : StatMods)
                {
                    if (Mod.Statistic.MatchesTag(Health))
                        EffectDamage += Mod.Value;
                }
                OnStatModificationsApplied_Implementation(StatMods);
                OnStatModificationsApplied(StatMods);
            }
        }
        break;
    case EStatusEffectApplicationMode::DamageEvent:
        {
            if (Config->DamageTypeClass && Config->DamageStatisticMods.Num() > 0)
            {
                for (const FStatisticValue& Mod : Config->DamageStatisticMods)
                {
                    if (Mod.Statistic.MatchesTag(Health) && !FMath::IsNearlyZero(Mod.Value))
                    {
                        UGameplayStatics::ApplyDamage(
                            Target,
                            FMath::Abs(Mod.Value),
                            nullptr,
                            Causer,
                            Config->DamageTypeClass
                        );
                        EffectDamage += Mod.Value;
                    }
                }
                OnStatModificationsApplied_Implementation(StatMods);
                OnStatModificationsApplied(StatMods);
            }
        }
        break;
    case EStatusEffectApplicationMode::Both:
        {
            UARSStatisticsComponent* StatsComp = Target->FindComponentByClass<UARSStatisticsComponent>();
            if (StatsComp)
            {
                UNomadStatusEffectUtils::ApplyStatModifications(StatsComp, StatMods);
            }
            if (Config->DamageTypeClass)
            {
                for (const FStatisticValue& Mod : StatMods)
                {
                    if (Mod.Statistic.MatchesTag(Health) && !FMath::IsNearlyZero(Mod.Value))
                    {
                        UGameplayStatics::ApplyDamage(
                            Target,
                            FMath::Abs(Mod.Value),
                            nullptr,
                            Causer,
                            Config->DamageTypeClass
                        );
                        EffectDamage += Mod.Value;
                    }
                }
            }
            OnStatModificationsApplied_Implementation(StatMods);
            OnStatModificationsApplied(StatMods);
        }
        break;
    default:
        break;
    }

    LastTickDamage = EffectDamage;

    if (Config->ApplicationMode != EStatusEffectApplicationMode::StatModification && !FMath::IsNearlyZero(EffectDamage))
    {
        if (UActorComponent* Comp = Target->GetComponentByClass(UNomadStatusEffectManagerComponent::StaticClass()))
        {
            if (auto* SEManager = Cast<UNomadStatusEffectManagerComponent>(Comp))
            {
                SEManager->AddStatusEffectDamage(Config->EffectTag, EffectDamage);
            }
        }
    }
}

void UNomadInfiniteStatusEffect::ApplyAttributeSetModifier()
{
    const UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (!Config || !CharacterOwner)
        return;

    const FAttributesSetModifier& Mod = Config->PersistentAttributeModifier;
    if (Mod.PrimaryAttributesMod.Num() == 0 && Mod.AttributesMod.Num() == 0 && Mod.StatisticsMod.Num() == 0)
        return;

    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp)
        return;

    AppliedModifierGuid = Mod.Guid;
    StatsComp->AddAttributeSetModifier(Mod);

    // Fire BP event for each modifier (primary, attributes, statistics)
    for (const FAttributeModifier& AttrMod : Mod.PrimaryAttributesMod)
    {
        OnPersistentAttributeApplied(AttrMod);
    }
    for (const FAttributeModifier& AttrMod : Mod.AttributesMod)
    {
        OnPersistentAttributeApplied(AttrMod);
    }
    // Optionally for FStatisticsModifier as well if you want, add a similar BP event
}

void UNomadInfiniteStatusEffect::RemoveAttributeSetModifier()
{
    if (!CharacterOwner || !AppliedModifierGuid.IsValid())
        return;

    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp)
        return;

    const UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config)
    {
        const FAttributesSetModifier& Mod = Config->PersistentAttributeModifier;
        StatsComp->RemoveAttributeSetModifier(Mod);

        // Fire BP event for each modifier being removed
        for (const FAttributeModifier& AttrMod : Mod.PrimaryAttributesMod)
        {
            OnPersistentAttributeRemoved(AttrMod);
        }
        for (const FAttributeModifier& AttrMod : Mod.AttributesMod)
        {
            OnPersistentAttributeRemoved(AttrMod);
        }
        // Optionally for FStatisticsModifier as well
    }

    AppliedModifierGuid = FGuid();
}

void UNomadInfiniteStatusEffect::CacheConfigurationValues()
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (!Config)
        return;
    CachedTickInterval = Config->TickInterval;
    bCachedHasPeriodicTick = Config->bHasPeriodicTick;
}

int32 UNomadInfiniteStatusEffect::GetCurrentStackCount() const
{
    if (!CharacterOwner) return StackCount;
    UNomadStatusEffectManagerComponent* Manager = CharacterOwner->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    if (!Manager) return StackCount;
    FGameplayTag Tag = GetEffectConfig() ? GetEffectConfig()->EffectTag : FGameplayTag();
    int32 Index = Manager->FindActiveEffectIndexByTag(Tag);
    if (Index != INDEX_NONE)
        return Manager->GetActiveEffects()[Index].StackCount;
    return StackCount;
}