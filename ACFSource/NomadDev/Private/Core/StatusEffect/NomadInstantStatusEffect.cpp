#include "Core/StatusEffect/NomadInstantStatusEffect.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "ARSStatisticsComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Core/StatusEffect/Utility/NomadStatusEffectUtils.h"
#include "Kismet/GameplayStatics.h"

UNomadInstantStatusEffect::UNomadInstantStatusEffect()
{
    ActivationTime = 0.0f;
    AppliedModifierGuid = FGuid();
    EffectState = EEffectLifecycleState::Removed;
    LastAppliedDamage = 0.0f;
    DamageCauser = nullptr;
}

UNomadInstantEffectConfig* UNomadInstantStatusEffect::GetConfig() const
{
    return EffectConfig.IsNull() ? nullptr : EffectConfig.LoadSynchronous();
}

void UNomadInstantStatusEffect::OnStatusEffectTriggered(ACharacter* Character, UNomadStatusEffectManagerComponent* Manager)
{
    CharacterOwner = Character;
    OwningManager = Manager;
    OnStatusEffectStarts_Implementation(Character);
}

void UNomadInstantStatusEffect::Nomad_OnStatusEffectStarts(ACharacter* Character)
{
    OnStatusEffectStarts_Implementation(Character);
}

void UNomadInstantStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    Super::OnStatusEffectStarts_Implementation(Character);
    EffectState = EEffectLifecycleState::Active;

    UNomadInstantEffectConfig* Config = GetConfig();
    if (!Config || !CharacterOwner) return;

    ActivationTime = CharacterOwner->GetWorld()->GetTimeSeconds();

    // Apply stat/damage/both modifications instantly
    ApplyHybridEffect(Config->OnApplyStatModifications, CharacterOwner, Config);

    // Cosmetic Blueprint hook
    OnInstantEffectTriggered(Character);

    // Only apply persistent attribute modifier if mode is StatModification or Both
    if (Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent) {
        ApplyAttributeSetModifier();
        if (Config->AttributeModifier.PrimaryAttributesMod.Num() > 0 ||
            Config->AttributeModifier.AttributesMod.Num() > 0 ||
            Config->AttributeModifier.StatisticsMod.Num() > 0) {
            OnInstantEffectAttributeModifierApplied(Config->AttributeModifier);
        }
    }

    // Immediately end the effect after applying everything
    OnStatusEffectEnds_Implementation();
}

void UNomadInstantStatusEffect::OnStatusEffectEnds_Implementation()
{
    // Only remove persistent attribute set modifier if it was applied
    UNomadInstantEffectConfig* Config = GetConfig();
    if (Config && Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent) {
        RemoveAttributeSetModifier();
    }

    EffectState = EEffectLifecycleState::Removed;
    Super::OnStatusEffectEnds_Implementation();
}

void UNomadInstantStatusEffect::ApplyStatModifications(const TArray<FStatisticValue>& Modifications)
{
    // Deprecated: Use ApplyHybridEffect for hybrid stat/damage support.
    ApplyHybridEffect(Modifications, CharacterOwner, GetConfig());
}

void UNomadInstantStatusEffect::ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfigObj)
{
    // --- SAFETY: Always check validity ---
    if (!IsValid(Target) || Target->IsPendingKillPending() || !EffectConfigObj)
        return;

    UNomadInstantEffectConfig* Config = Cast<UNomadInstantEffectConfig>(EffectConfigObj);
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
                // Cosmetic BP hook
                OnInstantEffectStatModificationsApplied(StatMods);
            }
        }
        break;
    case EStatusEffectApplicationMode::DamageEvent:
        {
            // Only damage applies! NO attribute/stat mods.
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
                // Cosmetic BP hook (stat mods not applied, but report for analytics)
                OnInstantEffectStatModificationsApplied(StatMods);
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
            // Cosmetic BP hook
            OnInstantEffectStatModificationsApplied(StatMods);
        }
        break;
    default:
        break;
    }

    LastAppliedDamage = EffectDamage;

    // Only add to analytics if there was actual damage (not for stat-only mode)
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

void UNomadInstantStatusEffect::ApplyAttributeSetModifier()
{
    UNomadInstantEffectConfig* Config = GetConfig();
    if (!Config || !CharacterOwner) return;
    if (Config->AttributeModifier.PrimaryAttributesMod.Num() == 0 &&
        Config->AttributeModifier.AttributesMod.Num() == 0 &&
        Config->AttributeModifier.StatisticsMod.Num() == 0)
        return;
    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp) return;
    AppliedModifierGuid = Config->AttributeModifier.Guid;
    StatsComp->AddAttributeSetModifier(Config->AttributeModifier);

    OnInstantEffectAttributeModifierApplied(Config->AttributeModifier);
}

void UNomadInstantStatusEffect::RemoveAttributeSetModifier()
{
    if (!CharacterOwner || !AppliedModifierGuid.IsValid()) return;
    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp) return;
    UNomadInstantEffectConfig* Config = GetConfig();
    if (Config) StatsComp->RemoveAttributeSetModifier(Config->AttributeModifier);
    AppliedModifierGuid = FGuid();
}

void UNomadInstantStatusEffect::TriggerChainEffects(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects)
{
    OnInstantEffectChainEffectsTriggered(ChainEffects);
}