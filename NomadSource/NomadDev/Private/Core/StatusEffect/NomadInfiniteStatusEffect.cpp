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
    // Initialize timing settings
    CachedTickInterval = 5.0f;
    bCachedHasPeriodicTick = false;
    StartTime = 0.0f;
    TickCount = 0;
    
    // Initialize modifier tracking
    AppliedModifierGuid = FGuid();
    LastTickDamage = 0.0f;
    StackCount = 1;
    
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[INFINITE] Infinite status effect constructed"));
}

// =====================================================
//         STACKING / REFRESH LOGIC
// =====================================================

void UNomadInfiniteStatusEffect::OnStacked_Implementation(const int32 NewStackCount)
{
    // Called when the effect is stacked (multiple applications).
    StackCount = NewStackCount;
    
    // Refresh persistent modifiers with new stack count
    RemoveAttributeSetModifier();
    ApplyAttributeSetModifier();
    
    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Effect stacked to %d"), NewStackCount);
}

void UNomadInfiniteStatusEffect::OnRefreshed_Implementation()
{
    // Called when the effect is refreshed (reapplied at max stacks).
    // For infinite effects, this typically just resets any decay or applies fresh modifiers
    RemoveAttributeSetModifier();
    ApplyAttributeSetModifier();
    
    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Effect refreshed"));
}

void UNomadInfiniteStatusEffect::OnUnstacked(int32 NewStackCount)
{
    // Called by manager when a stack is removed.
    StackCount = NewStackCount;
    
    if (StackCount > 0)
    {
        // Update persistent modifiers for new stack count
        RemoveAttributeSetModifier();
        ApplyAttributeSetModifier();
        
        UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Effect unstacked to %d"), NewStackCount);
    }
    // If StackCount is 0, the manager will destroy this instance
}

// =====================================================
//         CONFIGURATION ACCESS
// =====================================================

UNomadInfiniteEffectConfig* UNomadInfiniteStatusEffect::GetEffectConfig() const
{
    return Cast<UNomadInfiniteEffectConfig>(EffectConfig.IsNull() ? nullptr : EffectConfig.LoadSynchronous());
}

void UNomadInfiniteStatusEffect::ApplyConfiguration()
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (!Config) 
    {
        UE_LOG_AFFLICTION(Error, TEXT("[INFINITE] Cannot apply configuration - config is null"));
        return;
    }
    
    if (!Config->IsConfigValid()) 
    {
        UE_LOG_AFFLICTION(Error, TEXT("[INFINITE] Configuration validation failed"));
        return;
    }

    CacheConfigurationValues();
    ApplyBaseConfiguration();
    ApplyConfigurationTag();
    ApplyConfigurationIcon();
    
    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Configuration applied successfully"));
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

// =====================================================
//         QUERY FUNCTIONS
// =====================================================

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
//         MANUAL/FORCED CONTROL
// =====================================================

bool UNomadInfiniteStatusEffect::TryManualRemoval(AActor* Remover)
{
    if (!CanBeManuallyRemoved())
    {
        UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Manual removal not allowed"));
        return false;
    }

    // Check if removal is allowed via Blueprint
    bool bAllowRemoval = OnManualRemovalAttempt_Implementation(Remover);
    if (!bAllowRemoval)
    {
        // Give Blueprint a chance to override
        OnManualRemovalAttempt(Remover);
        if (!bAllowRemoval)
        {
            UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Manual removal denied by Blueprint logic"));
            return false;
        }
    }
    
    // Remove through manager if available
    if (CharacterOwner)
    {
        if (UNomadStatusEffectManagerComponent* Manager = CharacterOwner->FindComponentByClass<UNomadStatusEffectManagerComponent>())
        {
            Manager->Nomad_RemoveStatusEffect(GetEffectiveTag());
            UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Manual removal successful via manager"));
            return true;
        }
    }
    
    // Fallback to direct removal
    EndEffect();
    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Manual removal successful via direct end"));
    return true;
}

void UNomadInfiniteStatusEffect::ForceRemoval()
{
    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Force removal initiated"));
    EndEffect();
}

void UNomadInfiniteStatusEffect::Nomad_OnStatusEffectStarts(ACharacter* Character)
{
    Super::Nomad_OnStatusEffectStarts(Character);
    OnStatusEffectStarts_Implementation(Character);
}

// =====================================================
//         EFFECT LIFECYCLE: START / END
// =====================================================

void UNomadInfiniteStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    Super::OnStatusEffectStarts_Implementation(Character);
    SetEffectLifecycleState(EEffectLifecycleState::Active);

    if (!Character || !Character->GetWorld())
    {
        UE_LOG_AFFLICTION(Error, TEXT("[INFINITE] Cannot start - invalid character or world"));
        return;
    }

    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (!Config)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[INFINITE] Cannot start - invalid config"));
        return;
    }

    // Initialize timing
    StartTime = Character->GetWorld()->GetTimeSeconds();
    TickCount = 0;
    int32 CurrentStacks = GetCurrentStackCount();
    StackCount = CurrentStacks;

    // Apply activation stat modifications, scaled by stack count
    if (Config->OnActivationStatModifications.Num() > 0)
    {
        TArray<FStatisticValue> ScaledMods = Config->OnActivationStatModifications;
        for (FStatisticValue& Mod : ScaledMods)
        {
            Mod.Value *= CurrentStacks;
        }
        ApplyHybridEffect(ScaledMods, Character, Config);
        OnStatModificationsApplied_Implementation(ScaledMods);
        OnStatModificationsApplied(ScaledMods);
    }

    // Apply persistent attribute modifiers
    ApplyAttributeSetModifier();
    
    // Setup periodic ticking if enabled
    SetupInfiniteTicking();

    // Trigger chain effects if configured
    if (Config->bTriggerActivationChainEffects && Config->ActivationChainEffects.Num() > 0)
    {
        // TODO: Implement chain effect triggering through manager
        UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Activation chain effects triggered"));
    }

    // Trigger Blueprint events
    OnInfiniteEffectActivated_Implementation(Character);
    OnInfiniteEffectActivated(Character);
    
    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Effect started successfully with %d stacks"), CurrentStacks);
}

void UNomadInfiniteStatusEffect::OnStatusEffectEnds_Implementation()
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    
    // Apply deactivation stat modifications
    if (Config && Config->OnDeactivationStatModifications.Num() > 0)
    {
        if (GetEffectLifecycleState() != EEffectLifecycleState::Active && 
            GetEffectLifecycleState() != EEffectLifecycleState::Ending)
        {
            UE_LOG_AFFLICTION(Warning, TEXT("[INFINITE] End called on non-active effect"));
            return;
        }

        ApplyHybridEffect(Config->OnDeactivationStatModifications, CharacterOwner, Config);
        OnStatModificationsApplied_Implementation(Config->OnDeactivationStatModifications);
        OnStatModificationsApplied(Config->OnDeactivationStatModifications);
    }

    // Trigger deactivation chain effects
    if (Config && Config->bTriggerDeactivationChainEffects && Config->DeactivationChainEffects.Num() > 0)
    {
        // TODO: Implement chain effect triggering through manager
        UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Deactivation chain effects triggered"));
    }

    // Remove persistent modifiers
    RemoveAttributeSetModifier();
    
    // Clear ticking
    ClearInfiniteTicking();
    
    // Trigger Blueprint events
    OnInfiniteEffectDeactivated_Implementation();
    OnInfiniteEffectDeactivated();

    Super::OnStatusEffectEnds_Implementation();
    
    UE_LOG_AFFLICTION(Log, TEXT("[INFINITE] Effect ended successfully"));
}

// =====================================================
//         DEFAULT BLUEPRINT IMPLEMENTATIONS
// =====================================================

void UNomadInfiniteStatusEffect::OnInfiniteEffectActivated_Implementation(ACharacter* Character)
{
    // Default implementation - override in Blueprint or derived classes
}

void UNomadInfiniteStatusEffect::OnInfiniteTick_Implementation(float Uptime, int32 CurrentTickCount)
{
    // Default implementation - override in Blueprint or derived classes
}

bool UNomadInfiniteStatusEffect::OnManualRemovalAttempt_Implementation(AActor* Remover)
{
    return CanBeManuallyRemoved();
}

void UNomadInfiniteStatusEffect::OnInfiniteEffectDeactivated_Implementation()
{
    // Default implementation - override in Blueprint or derived classes
}

void UNomadInfiniteStatusEffect::OnStatModificationsApplied_Implementation(const TArray<FStatisticValue>& StatisticModifications)
{
    // Default implementation - override in Blueprint or derived classes
}

// =====================================================
//         TIMER MANAGEMENT (PERIODIC TICK)
// =====================================================

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
        true // Repeating
    );
    
    UE_LOG_AFFLICTION(Verbose, TEXT("[INFINITE] Periodic ticking setup with %.2f second intervals"), CachedTickInterval);
}

void UNomadInfiniteStatusEffect::ClearInfiniteTicking()
{
    if (!CharacterOwner || !CharacterOwner->GetWorld())
        return;

    UWorld* World = CharacterOwner->GetWorld();
    FTimerManager& TimerManager = World->GetTimerManager();
    TimerManager.ClearTimer(TickTimerHandle);
    
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[INFINITE] Periodic ticking cleared"));
}

void UNomadInfiniteStatusEffect::HandleInfiniteTick()
{
    if (GetEffectLifecycleState() != EEffectLifecycleState::Active)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[INFINITE] Tick called on non-active effect"));
        return;
    }

    TickCount++;
    float CurrentUptime = GetUptime();
    int32 CurrentStacks = GetCurrentStackCount();
    StackCount = CurrentStacks;

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[INFINITE] Tick %d (uptime: %.1fs, stacks: %d)"), 
                      TickCount, CurrentUptime, CurrentStacks);

    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config && Config->OnTickStatModifications.Num() > 0)
    {
        // Apply tick modifications scaled by stack count
        TArray<FStatisticValue> ScaledMods = Config->OnTickStatModifications;
        for (FStatisticValue& Mod : ScaledMods)
        {
            Mod.Value *= CurrentStacks;
        }
        ApplyHybridEffect(ScaledMods, CharacterOwner, Config);
        OnStatModificationsApplied_Implementation(ScaledMods);
        OnStatModificationsApplied(ScaledMods);
    }

    // Trigger Blueprint events
    OnInfiniteTick_Implementation(CurrentUptime, TickCount);
    OnInfiniteTick(CurrentUptime, TickCount);
}

// =====================================================
//         HYBRID SYSTEM: STAT/DAMAGE APPLICATION
// =====================================================

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
        }
        break;
    default:
        break;
    }

    LastTickDamage = EffectDamage;

    // Record damage analytics
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

// =====================================================
//         MODIFIER HELPERS
// =====================================================

void UNomadInfiniteStatusEffect::ApplyAttributeSetModifier()
{
    const UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (!Config || !CharacterOwner) return;

    const FAttributesSetModifier& Mod = Config->PersistentAttributeModifier;
    if (Mod.PrimaryAttributesMod.Num() == 0 && 
        Mod.AttributesMod.Num() == 0 && 
        Mod.StatisticsMod.Num() == 0)
    {
        return;
    }

    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp) return;

    // Create scaled modifier based on stack count
    FAttributesSetModifier ScaledModifier = Mod;
    for (FAttributeModifier& AttrMod : ScaledModifier.PrimaryAttributesMod)
    {
        AttrMod.Value *= StackCount;
    }
    for (FAttributeModifier& AttrMod : ScaledModifier.AttributesMod)
    {
        AttrMod.Value *= StackCount;
    }
    for (FStatisticsModifier& StatMod : ScaledModifier.StatisticsMod)
    {
        StatMod.MaxValue *= StackCount;
        StatMod.RegenValue *= StackCount;
    }

    AppliedModifierGuid = ScaledModifier.Guid;
    StatsComp->AddAttributeSetModifier(ScaledModifier);

    // Fire Blueprint events for each modifier
    for (const FAttributeModifier& AttrMod : ScaledModifier.PrimaryAttributesMod)
    {
        OnPersistentAttributeApplied(AttrMod);
    }
    for (const FAttributeModifier& AttrMod : ScaledModifier.AttributesMod)
    {
        OnPersistentAttributeApplied(AttrMod);
    }
    
    UE_LOG_AFFLICTION(Verbose, TEXT("[INFINITE] Applied persistent attribute modifier with %d stacks"), StackCount);
}

void UNomadInfiniteStatusEffect::RemoveAttributeSetModifier()
{
    if (!CharacterOwner || !AppliedModifierGuid.IsValid()) return;

    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp) return;

    const UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config)
    {
        const FAttributesSetModifier& Mod = Config->PersistentAttributeModifier;
        StatsComp->RemoveAttributeSetModifier(Mod);

        // Fire Blueprint events for each modifier being removed
        for (const FAttributeModifier& AttrMod : Mod.PrimaryAttributesMod)
        {
            OnPersistentAttributeRemoved(AttrMod);
        }
        for (const FAttributeModifier& AttrMod : Mod.AttributesMod)
        {
            OnPersistentAttributeRemoved(AttrMod);
        }
    }

    AppliedModifierGuid = FGuid();
    UE_LOG_AFFLICTION(Verbose, TEXT("[INFINITE] Removed persistent attribute modifier"));
}

void UNomadInfiniteStatusEffect::CacheConfigurationValues()
{
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (!Config) return;
    
    CachedTickInterval = Config->TickInterval;
    bCachedHasPeriodicTick = Config->bHasPeriodicTick;
    
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[INFINITE] Configuration values cached"));
}

int32 UNomadInfiniteStatusEffect::GetCurrentStackCount() const
{
    if (!CharacterOwner) return StackCount;
    
    UNomadStatusEffectManagerComponent* Manager = CharacterOwner->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    if (!Manager) return StackCount;
    
    const UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (!Config) return StackCount;
    
    const int32 Index = Manager->FindActiveEffectIndexByTag(Config->EffectTag);
    if (Index != INDEX_NONE)
    {
        return Manager->GetActiveEffects()[Index].StackCount;
    }
    
    return StackCount;
}