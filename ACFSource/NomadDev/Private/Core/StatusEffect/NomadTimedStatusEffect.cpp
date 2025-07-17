#include "Core/StatusEffect/NomadTimedStatusEffect.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "ARSStatisticsComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Core/StatusEffect/Utility/NomadStatusEffectUtils.h"
#include "Kismet/GameplayStatics.h"

UNomadTimedStatusEffect::UNomadTimedStatusEffect()
{
    StartTime = 0.0f;
    CurrentTickCount = 0;
    AppliedModifierGuid = FGuid();
    EffectState = EEffectLifecycleState::Removed;
    LastTickDamage = 0.0f;
    DamageCauser = nullptr;
}

UNomadTimedEffectConfig* UNomadTimedStatusEffect::GetConfig() const
{
    return EffectConfig.IsNull() ? nullptr : EffectConfig.LoadSynchronous();
}

void UNomadTimedStatusEffect::OnStatusEffectStarts(ACharacter* Character, UNomadStatusEffectManagerComponent* Manager)
{
    CharacterOwner = Character;
    OwningManager = Manager;
    OnStatusEffectStarts_Implementation(Character);
}

void UNomadTimedStatusEffect::OnStatusEffectEnds()
{
    OnStatusEffectEnds_Implementation();
}

void UNomadTimedStatusEffect::RestartTimerIfStacking()
{
    ClearTimers();
    SetupTimers();
}

void UNomadTimedStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    Super::OnStatusEffectStarts_Implementation(Character);
    EffectState = EEffectLifecycleState::Active;

    UNomadTimedEffectConfig* Config = GetConfig();
    if (!Config || !CharacterOwner) return;

    StartTime = CharacterOwner->GetWorld()->GetTimeSeconds();
    CurrentTickCount = 0;

    // Apply stat/damage/both modifications defined for effect start.
    ApplyHybridEffect(Config->OnStartStatModifications, CharacterOwner, Config);

    // Cosmetic Blueprint hook: effect started
    OnTimedEffectStarted(Character);

    // Only apply persistent attribute/primary/stat modifiers in correct modes
    if (Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent)
        ApplyAttributeSetModifier();

    if (Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent &&
        (Config->AttributeModifier.PrimaryAttributesMod.Num() > 0 ||
         Config->AttributeModifier.AttributesMod.Num() > 0 ||
         Config->AttributeModifier.StatisticsMod.Num() > 0))
    {
        OnTimedEffectAttributeModifierApplied(Config->AttributeModifier);
    }

    SetupTimers();
}

void UNomadTimedStatusEffect::SetupTimers()
{
    UNomadTimedEffectConfig* Config = GetConfig();
    if (!Config || !CharacterOwner || !CharacterOwner->GetWorld()) return;

    UWorld* World = CharacterOwner->GetWorld();
    FTimerManager& TimerManager = World->GetTimerManager();

    float EndTime = 0.f;

    if (Config->bIsPeriodic)
    {
        if (Config->DurationMode == EEffectDurationMode::Duration)
        {
            EndTime = Config->EffectDuration;
        }
        else
        {
            EndTime = Config->TickInterval * Config->NumTicks;
        }
    }
    else
    {
        EndTime = Config->EffectDuration;
    }

    if (EndTime > 0.0f)
        TimerManager.SetTimer(TimerHandle_End, this, &UNomadTimedStatusEffect::HandleEnd, EndTime, false);

    if (Config->bIsPeriodic)
    {
        TimerManager.SetTimer(TimerHandle_Tick, this, &UNomadTimedStatusEffect::HandleTick, Config->TickInterval, true);
    }
}

void UNomadTimedStatusEffect::ClearTimers()
{
    if (!CharacterOwner || !CharacterOwner->GetWorld()) return;
    UWorld* World = CharacterOwner->GetWorld();
    FTimerManager& TimerManager = World->GetTimerManager();
    TimerManager.ClearTimer(TimerHandle_End);
    TimerManager.ClearTimer(TimerHandle_Tick);
}

void UNomadTimedStatusEffect::HandleTick()
{
    UNomadTimedEffectConfig* Config = GetConfig();
    if (!Config) return;

    ++CurrentTickCount;

    ApplyHybridEffect(Config->OnTickStatModifications, CharacterOwner, Config);

    OnTimedEffectTicked(CurrentTickCount);

    if (Config->bIsPeriodic && Config->DurationMode == EEffectDurationMode::Ticks && CurrentTickCount >= Config->NumTicks)
    {
        HandleEnd();
    }
}

void UNomadTimedStatusEffect::HandleEnd()
{
    ClearTimers();

    UNomadTimedEffectConfig* Config = GetConfig();

    if (OwningManager && Config)
    {
        OwningManager->RemoveStatusEffect(Config->EffectTag);
        return;
    }

    OnStatusEffectEnds_Implementation();
}

void UNomadTimedStatusEffect::OnStatusEffectEnds_Implementation()
{
    UNomadTimedEffectConfig* Config = GetConfig();
    if (Config)
    {
        if (EffectState != EEffectLifecycleState::Active && EffectState != EEffectLifecycleState::Ending)
            return;
        
        ApplyHybridEffect(Config->OnEndStatModifications, CharacterOwner, Config);

        // Only remove persistent attribute set modifier if it was applied
        if (Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent)
            RemoveAttributeSetModifier();
    }

    OnTimedEffectEnded();
    
    EffectState = EEffectLifecycleState::Removed;
    Super::OnStatusEffectEnds_Implementation();
}

void UNomadTimedStatusEffect::ApplyStatModifications(const TArray<FStatisticValue>& Modifications)
{
    ApplyHybridEffect(Modifications, CharacterOwner, GetConfig());
}

void UNomadTimedStatusEffect::ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfigObj)
{
    if (!IsValid(Target) || Target->IsPendingKillPending() || !EffectConfigObj)
        return;

    UNomadTimedEffectConfig* Config = Cast<UNomadTimedEffectConfig>(EffectConfigObj);
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
                OnTimedEffectStatModificationsApplied(StatMods);
            }
        }
        break;
    case EStatusEffectApplicationMode::DamageEvent:
        {
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
                }
                OnTimedEffectStatModificationsApplied(StatMods);
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
            OnTimedEffectStatModificationsApplied(StatMods);
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

void UNomadTimedStatusEffect::ApplyAttributeSetModifier()
{
    UNomadTimedEffectConfig* Config = GetConfig();
    if (!Config || !CharacterOwner) return;
    if (Config->AttributeModifier.PrimaryAttributesMod.Num() == 0 &&
        Config->AttributeModifier.AttributesMod.Num() == 0 &&
        Config->AttributeModifier.StatisticsMod.Num() == 0)
        return;
    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp) return;
    AppliedModifierGuid = Config->AttributeModifier.Guid;
    StatsComp->AddAttributeSetModifier(Config->AttributeModifier);

    OnTimedEffectAttributeModifierApplied(Config->AttributeModifier);
}

void UNomadTimedStatusEffect::RemoveAttributeSetModifier()
{
    if (!CharacterOwner || !AppliedModifierGuid.IsValid()) return;
    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp) return;
    UNomadTimedEffectConfig* Config = GetConfig();
    if (Config) StatsComp->RemoveAttributeSetModifier(Config->AttributeModifier);
    AppliedModifierGuid = FGuid();
}

void UNomadTimedStatusEffect::TriggerChainEffects(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects)
{
    OnTimedEffectChainEffectsTriggered(ChainEffects);
}