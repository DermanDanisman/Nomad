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

UNomadInfiniteStatusEffect::UNomadInfiniteStatusEffect()
{
    CachedTickInterval = 5.0f;
    bCachedHasPeriodicTick = false;
    StartTime = 0.0f;
    TickCount = 0;
    AppliedModifierGuid = FGuid();
    EffectState = EEffectLifecycleState::Removed;
    LastTickDamage = 0.0f;
}

UNomadInfiniteEffectConfig* UNomadInfiniteStatusEffect::GetEffectConfig() const
{
    if (EffectConfig.IsNull())
    {
        return nullptr;
    }
    return EffectConfig.LoadSynchronous();
}

void UNomadInfiniteStatusEffect::ApplyConfiguration()
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    checkf(Config, TEXT("Infinite Status Effect Config Data Asset is empty"));

    if (!Config->IsConfigValid())
    {
        UE_LOG_AFFLICTION(Error, TEXT("[INFINITE] Configuration validation failed for effect"));
        return;
    }

    CacheConfigurationValues();
    ApplyBaseConfiguration();
    ApplyConfigurationTag();
    ApplyConfigurationIcon();

    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Configuration applied: %s (Infinite Duration)"),
        *Config->EffectName.ToString());
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
        UE_LOG_AFFLICTION(Verbose, TEXT("[INFINITE] Applied tag from config: %s"), *Config->EffectTag.ToString());
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
            UE_LOG_AFFLICTION(Verbose, TEXT("[INFINITE] Applied icon from config"));
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
    {
        return 0.0f;
    }
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

bool UNomadInfiniteStatusEffect::TryManualRemoval(AActor* Remover)
{
    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Manual removal attempt by %s"),
        Remover ? *Remover->GetName() : TEXT("Unknown"));

    if (!CanBeManuallyRemoved())
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[INFINITE] Manual removal not allowed for this effect"));
        return false;
    }

    bool bAllowRemoval = OnManualRemovalAttempt_Implementation(Remover);
    if (!bAllowRemoval)
    {
        OnManualRemovalAttempt(Remover);
        if (!bAllowRemoval)
        {
            UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Manual removal denied by Blueprint logic"));
            return false;
        }
    }

    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Manual removal approved, ending effect"));
    EndEffect();
    return true;
}

void UNomadInfiniteStatusEffect::ForceRemoval()
{
    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Force removal initiated"));
    EndEffect();
}

void UNomadInfiniteStatusEffect::Nomad_OnStatusEffectStarts(ACharacter* Character)
{
    OnStatusEffectStarts_Implementation(Character);
}

void UNomadInfiniteStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    ApplyConfiguration();

    Super::OnStatusEffectStarts_Implementation(Character);
    EffectState = EEffectLifecycleState::Active;

    if (Character && Character->GetWorld())
    {
        StartTime = Character->GetWorld()->GetTimeSeconds();
        TickCount = 0;

        UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Effect started - will persist until manually removed"));

        UNomadInfiniteEffectConfig* Config = GetEffectConfig();
        if (Config && Config->OnActivationStatModifications.Num() > 0)
        {
            ApplyHybridEffect(Config->OnActivationStatModifications, Character, Config);
            OnStatModificationsApplied_Implementation(Config->OnActivationStatModifications);
            OnStatModificationsApplied(Config->OnActivationStatModifications);
        }

        ApplyAttributeSetModifier();
        SetupInfiniteTicking();

        OnInfiniteEffectActivated_Implementation(Character);
        OnInfiniteEffectActivated(Character);
    }
}

void UNomadInfiniteStatusEffect::OnStatusEffectEnds_Implementation()
{
    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Effect ended after %.1f seconds uptime"), GetUptime());

    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config && Config->OnDeactivationStatModifications.Num() > 0)
    {
        if (EffectState != EEffectLifecycleState::Active && EffectState != EEffectLifecycleState::Ending)
            return;

        ApplyHybridEffect(Config->OnDeactivationStatModifications, CharacterOwner, Config);
        OnStatModificationsApplied_Implementation(Config->OnDeactivationStatModifications);
        OnStatModificationsApplied(Config->OnDeactivationStatModifications);
    }

    RemoveAttributeSetModifier();
    ClearInfiniteTicking();
    OnInfiniteEffectDeactivated_Implementation();
    OnInfiniteEffectDeactivated();

    EffectState = EEffectLifecycleState::Removed;
    Super::OnStatusEffectEnds_Implementation();
}

void UNomadInfiniteStatusEffect::SetupInfiniteTicking()
{
    if (!bCachedHasPeriodicTick || CachedTickInterval <= 0.0f || !CharacterOwner || !CharacterOwner->GetWorld())
    {
        return;
    }

    UWorld* World = CharacterOwner->GetWorld();
    FTimerManager& TimerManager = World->GetTimerManager();

    TimerManager.SetTimer(
        TickTimerHandle,
        this,
        &UNomadInfiniteStatusEffect::HandleInfiniteTick,
        CachedTickInterval,
        true
    );

    UE_LOG_AFFLICTION(Verbose, TEXT("[INFINITE] Periodic ticking set up: every %.1f seconds"), CachedTickInterval);
}

void UNomadInfiniteStatusEffect::ClearInfiniteTicking()
{
    if (!CharacterOwner || !CharacterOwner->GetWorld())
    {
        return;
    }

    UWorld* World = CharacterOwner->GetWorld();
    FTimerManager& TimerManager = World->GetTimerManager();

    TimerManager.ClearTimer(TickTimerHandle);

    UE_LOG_AFFLICTION(Verbose, TEXT("[INFINITE] Periodic ticking cleared"));
}

void UNomadInfiniteStatusEffect::HandleInfiniteTick()
{
    TickCount++;
    float CurrentUptime = GetUptime();

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[INFINITE] Tick #%d: %.1fs uptime"), TickCount, CurrentUptime);

    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config && Config->OnTickStatModifications.Num() > 0)
    {
        ApplyHybridEffect(Config->OnTickStatModifications, CharacterOwner, Config);
        OnStatModificationsApplied_Implementation(Config->OnTickStatModifications);
        OnStatModificationsApplied(Config->OnTickStatModifications);
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
            // Only stat/attribute mods apply! NO damage.
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
            // Only damage applies! NO stat/attribute mods.
            if (Config->DamageTypeClass)
            {
                if (Config->DamageStatisticMods.Num() > 0)
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
        }
        break;
    case EStatusEffectApplicationMode::Both:
        {
            // BOTH stat/attribute mods AND damage apply.
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

    // Only report analytics for damage (never for stat-only)
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

void UNomadInfiniteStatusEffect::ApplyStatModifications(const TArray<FStatisticValue>& Modifications)
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config)
        ApplyHybridEffect(Modifications, CharacterOwner, Config);
}

void UNomadInfiniteStatusEffect::ApplyAttributeSetModifier()
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (!Config || !CharacterOwner)
    {
        return;
    }

    if (Config->PersistentAttributeModifier.PrimaryAttributesMod.Num() == 0 &&
        Config->PersistentAttributeModifier.AttributesMod.Num() == 0 &&
        Config->PersistentAttributeModifier.StatisticsMod.Num() == 0)
    {
        return;
    }

    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[INFINITE] No statistics component found for persistent attribute modifier"));
        return;
    }

    AppliedModifierGuid = Config->PersistentAttributeModifier.Guid;
    StatsComp->AddAttributeSetModifier(Config->PersistentAttributeModifier);

    UE_LOG_AFFLICTION(Verbose, TEXT("[INFINITE] Applied persistent attribute set modifier"));
}

void UNomadInfiniteStatusEffect::RemoveAttributeSetModifier()
{
    if (!CharacterOwner || !AppliedModifierGuid.IsValid())
    {
        return;
    }

    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp)
    {
        return;
    }

    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config)
    {
        StatsComp->RemoveAttributeSetModifier(Config->PersistentAttributeModifier);
        UE_LOG_AFFLICTION(Verbose, TEXT("[INFINITE] Removed persistent attribute set modifier"));
    }

    AppliedModifierGuid = FGuid();
}

void UNomadInfiniteStatusEffect::CacheConfigurationValues()
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (!Config)
    {
        return;
    }

    CachedTickInterval = Config->TickInterval;
    bCachedHasPeriodicTick = Config->bHasPeriodicTick;

    UE_LOG_AFFLICTION(Verbose, TEXT("[INFINITE] Cached config values: TickInterval=%.1f, HasTick=%s"),
        CachedTickInterval, bCachedHasPeriodicTick ? TEXT("true") : TEXT("false"));
}