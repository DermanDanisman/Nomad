// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/SurvivalHazard/NomadHazardDoTStatusEffect.h"
#include "ARSStatisticsComponent.h"
#include "Core/Data/StatusEffect/NomadInfiniteEffectConfig.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

/**
 * UNomadHazardDoTStatusEffect
 * ----------------------------------------------------------
 * Infinite status effect for starvation and dehydration hazard damage-over-time (DoT).
 * - Damage per tick is set dynamically using SetDoTPercent.
 * - On each tick, all configured stat modifications (from asset) are applied.
 * - No stacking: Each instance is independent and non-stackable.
 * - Analytics and gameplay events for UI/audio triggers.
 */

UNomadHazardDoTStatusEffect::UNomadHazardDoTStatusEffect()
    : Super()
{
    // DoTPercent MUST be set by system on creation (not by config asset).
    DoTPercent = 0.0f;
}

void UNomadHazardDoTStatusEffect::SetDoTPercent(float InPercent)
{
    // Store the DoT percent value for use in damage calculation.
    DoTPercent = InPercent;
}

/**
 * Called automatically on each periodic tick (interval set by config).
 * Applies periodic health damage based on current DoTPercent and max health.
 * Also applies any designer-configured stat mods from the config asset.
 */
void UNomadHazardDoTStatusEffect::HandleInfiniteTick()
{
    // 1. Validate owner and percent
    if (!CharacterOwner || DoTPercent <= 0.f)
        return;

    // 2. Get statistics component
    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp)
        return;

    // 3. Get max health
    const FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.Health"));
    float MaxHealth = StatsComp->GetMaxValueForStatitstic(HealthTag);
    if (MaxHealth <= 0.f)
        return;

    // 4. Compute tick interval from config
    float TickInterval = GetEffectiveTickInterval(); // From base class/config asset (default 1s or 5s typically)

    // 5. Calculate damage: percent of max health per tick
    float Damage = MaxHealth * DoTPercent * TickInterval;

    // 6. Apply negative value to health stat
    StatsComp->ModifyStatistic(HealthTag, -FMath::Abs(Damage));
    LastTickDamage = Damage; // For analytics/UI

    // 7. Optionally apply additional stat mods from config (designer flexibility)
    UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (Config && Config->OnTickStatModifications.Num() > 0)
    {
        ApplyHybridEffect(Config->OnTickStatModifications, CharacterOwner, Config);
    }

#if !UE_BUILD_SHIPPING
    // 8. Debug log
    UE_LOG(LogTemp, Log, TEXT("[HazardDoT] %s took %.2f damage (%.2f%% max health, TickInterval: %.2fs)"),
        *CharacterOwner->GetName(), Damage, DoTPercent * 100.f, TickInterval);
#endif
}