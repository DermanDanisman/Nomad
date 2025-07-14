// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "Core/StatusEffect/NomadTimedStatusEffect.h"
#include "Core/Data/StatusEffect/NomadTimedEffectConfig.h"
#include "Core/Component/NomadAfflictionComponent.h"
#include "Core/StatusEffect/NomadInfiniteStatusEffect.h"
#include "Core/StatusEffect/NomadInstantStatusEffect.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "StatusEffects/ACFBaseStatusEffect.h"

// =====================================================
//         CONSTRUCTOR & REPLICATION SETUP
// =====================================================

UNomadStatusEffectManagerComponent::UNomadStatusEffectManagerComponent()
{
    TotalStatusEffectDamage = 0.0f;
    StatusEffectDamageTotals.Empty();
}

void UNomadStatusEffectManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UNomadStatusEffectManagerComponent, ActiveEffects);
}

// =====================================================
//         REPLICATION CALLBACK
// =====================================================

void UNomadStatusEffectManagerComponent::OnRep_ActiveEffects()
{
    // Called automatically on clients when ActiveEffects array is updated
    // (due to replication). Implement UI refresh, VFX updates, etc. here if needed.
}

void UNomadStatusEffectManagerComponent::AddBlockingTag(const FGameplayTag& Tag)
{
    ActiveBlockingTags.AddTag(Tag); // Adds or increments stack
}

void UNomadStatusEffectManagerComponent::RemoveBlockingTag(const FGameplayTag& Tag)
{
    ActiveBlockingTags.RemoveTag(Tag); // Removes or decrements stack
}

bool UNomadStatusEffectManagerComponent::HasBlockingTag(const FGameplayTag& Tag) const
{
    return ActiveBlockingTags.HasTag(Tag);
}

// =====================================================
//         LIFECYCLE MANAGEMENT
// =====================================================

void UNomadStatusEffectManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (const FActiveEffect& Eff : ActiveEffects)
    {
        if (Eff.EffectInstance)
        {
            Eff.EffectInstance->Nomad_OnStatusEffectEnds();
        }
    }
    ActiveEffects.Empty();
    Super::EndPlay(EndPlayReason);
}

// =====================================================
//         DAMAGE ANALYTICS
// =====================================================

void UNomadStatusEffectManagerComponent::AddStatusEffectDamage(const FGameplayTag EffectTag, const float Delta)
{
    TotalStatusEffectDamage += Delta;
    StatusEffectDamageTotals.FindOrAdd(EffectTag) += Delta;
}

float UNomadStatusEffectManagerComponent::GetTotalStatusEffectDamage() const
{
    return TotalStatusEffectDamage;
}

float UNomadStatusEffectManagerComponent::GetStatusEffectDamageByTag(const FGameplayTag EffectTag) const
{
    if (const float* Found = StatusEffectDamageTotals.Find(EffectTag))
    {
        return *Found;
    }
    return 0.0f;
}

TMap<FGameplayTag, float> UNomadStatusEffectManagerComponent::GetAllStatusEffectDamages() const
{
    return StatusEffectDamageTotals;
}

void UNomadStatusEffectManagerComponent::ResetStatusEffectDamageTracking()
{
    TotalStatusEffectDamage = 0.0f;
    StatusEffectDamageTotals.Empty();
}

// =====================================================
//         GETTERS / ACCESSORS
// =====================================================

int32 UNomadStatusEffectManagerComponent::FindActiveEffectIndexByTag(const FGameplayTag Tag) const
{
    return ActiveEffects.IndexOfByPredicate([&](const FActiveEffect& Eff) { return Eff.Tag == Tag; });
}

// =====================================================
//         MAIN EXTERNAL CONTROL
// =====================================================

void UNomadStatusEffectManagerComponent::Nomad_AddStatusEffect(const TSubclassOf<UACFBaseStatusEffect> StatusEffectClass, AActor* Instigator)
{
    CreateAndApplyStatusEffect(StatusEffectClass, Instigator);
}

void UNomadStatusEffectManagerComponent::Nomad_RemoveStatusEffect(const FGameplayTag StatusEffectTag)
{
    RemoveStatusEffect(StatusEffectTag);
}

// =====================================================
//         EFFECT LIFECYCLE (INTERNAL LOGIC)
// =====================================================

void UNomadStatusEffectManagerComponent::CreateAndApplyStatusEffect_Implementation(
    const TSubclassOf<UACFBaseStatusEffect> StatusEffectToConstruct, AActor* Instigator)
{
    if (!StatusEffectToConstruct) {
        UE_LOG(LogTemp, Warning, TEXT("StatusEffectToConstruct not set or invalid! - NomadStatusEffectManagerComponent"));
        return;
    }
    UObject* EffectCDO = StatusEffectToConstruct->GetDefaultObject();
    if (!EffectCDO) return;

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return;

    // --- INSTANT EFFECT ---
    if (EffectCDO->IsA(UNomadInstantStatusEffect::StaticClass()))
    {
        UNomadInstantStatusEffect* NewEffect = NewObject<UNomadInstantStatusEffect>(OwnerActor, StatusEffectToConstruct);
        if (NewEffect)
        {
            ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor);
            // Set causer to Instigator (preferred) or fallback to owner char
            NewEffect->SetDamageCauser( Instigator ? Instigator : OwnerChar);
            NewEffect->Nomad_OnStatusEffectStarts(OwnerChar);
        }
        return;
    }

    // --- TIMED EFFECT ---
    if (EffectCDO->IsA(UNomadTimedStatusEffect::StaticClass()))
    {
        const UNomadTimedStatusEffect* EffectCDO_Timed = Cast<UNomadTimedStatusEffect>(EffectCDO);
        const UNomadTimedEffectConfig* Config = EffectCDO_Timed ? EffectCDO_Timed->GetEffectConfig() : nullptr;
        if (!Config) return;
        const FGameplayTag EffectTag = Config->EffectTag;
        if (!EffectTag.IsValid()) return;

        const int32 Index = FindActiveEffectIndexByTag(EffectTag);
        if (Index != INDEX_NONE)
        {
            FActiveEffect& Eff = ActiveEffects[Index];
            const UNomadTimedEffectConfig* ActiveConfig = nullptr;
            if (Eff.EffectInstance)
            {
                ActiveConfig = Cast<UNomadTimedEffectConfig>(Cast<UNomadTimedStatusEffect>(Eff.EffectInstance)->GetEffectConfig());
            }
            if (ActiveConfig && ActiveConfig->bCanStack)
            {
                const int32 PrevStacks = Eff.StackCount;
                if (Eff.StackCount < ActiveConfig->MaxStackSize)
                {
                    Eff.StackCount++;
                    if (Eff.EffectInstance)
                    {
                        Cast<UNomadTimedStatusEffect>(Eff.EffectInstance)->OnStacked(Eff.StackCount);
                    }
                    NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Stacked, PrevStacks, Eff.StackCount);
                }
                else
                {
                    if (Eff.EffectInstance)
                    {
                        Cast<UNomadTimedStatusEffect>(Eff.EffectInstance)->OnRefreshed();
                    }
                    NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Refreshed, Eff.StackCount, Eff.StackCount);
                }
            }
            else
            {
                if (Eff.EffectInstance)
                {
                    Cast<UNomadTimedStatusEffect>(Eff.EffectInstance)->OnRefreshed();
                }
                NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Refreshed, Eff.StackCount, Eff.StackCount);
            }
            return;
        }

        UNomadTimedStatusEffect* NewEffect = NewObject<UNomadTimedStatusEffect>(OwnerActor, StatusEffectToConstruct);
        if (NewEffect)
        {
            ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor);
            NewEffect->SetDamageCauser( Instigator ? Instigator : OwnerChar);
            NewEffect->NomadStartEffectWithManager(OwnerChar, this);
            ActiveEffects.Add({EffectTag, 1, NewEffect});
            NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Applied, 0, 1);
        }
        return;
    }

    // --- INFINITE EFFECT ---
    if (EffectCDO->IsA(UNomadInfiniteStatusEffect::StaticClass()))
    {
        const UNomadInfiniteStatusEffect* EffectCDO_Inf = Cast<UNomadInfiniteStatusEffect>(EffectCDO);
        const UNomadInfiniteEffectConfig* Config = EffectCDO_Inf ? EffectCDO_Inf->GetEffectConfig() : nullptr;
        if (!Config) return;
        const FGameplayTag EffectTag = Config->EffectTag;
        if (!EffectTag.IsValid()) return;

        const int32 Index = FindActiveEffectIndexByTag(EffectTag);
        if (Index != INDEX_NONE)
        {
            FActiveEffect& Eff = ActiveEffects[Index];
            const UNomadInfiniteEffectConfig* ActiveConfig = nullptr;
            if (Eff.EffectInstance)
            {
                ActiveConfig = Cast<UNomadInfiniteEffectConfig>(Cast<UNomadInfiniteStatusEffect>(Eff.EffectInstance)->GetEffectConfig());
            }
            if (ActiveConfig && ActiveConfig->bCanStack)
            {
                const int32 PrevStacks = Eff.StackCount;
                if (Eff.StackCount < ActiveConfig->MaxStackSize)
                {
                    Eff.StackCount++;
                    if (Eff.EffectInstance)
                    {
                        Cast<UNomadInfiniteStatusEffect>(Eff.EffectInstance)->OnStacked(Eff.StackCount);
                    }
                    NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Stacked, PrevStacks, Eff.StackCount);
                }
                else
                {
                    if (Eff.EffectInstance)
                    {
                        Cast<UNomadInfiniteStatusEffect>(Eff.EffectInstance)->OnRefreshed();
                    }
                    NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Refreshed, Eff.StackCount, Eff.StackCount);
                }
            }
            else
            {
                if (Eff.EffectInstance)
                {
                    Cast<UNomadInfiniteStatusEffect>(Eff.EffectInstance)->OnRefreshed();
                }
                NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Refreshed, Eff.StackCount, Eff.StackCount);
            }
            return;
        }

        UNomadInfiniteStatusEffect* NewEffect = NewObject<UNomadInfiniteStatusEffect>(OwnerActor, StatusEffectToConstruct);
        if (NewEffect)
        {
            ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor);
            NewEffect->SetDamageCauser( Instigator ? Instigator : OwnerChar);
            NewEffect->Nomad_OnStatusEffectStarts(OwnerChar);
            ActiveEffects.Add({EffectTag, 1, NewEffect});
            NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Applied, 0, 1);
        }
        return;
    }
}

void UNomadStatusEffectManagerComponent::AddStatusEffect(UACFBaseStatusEffect* StatusEffect, AActor* Instigator)
{
    Super::AddStatusEffect(StatusEffect, GetOwner());
}

// =====================================================
//         STACK REMOVAL (UPDATED FOR DAMAGE ON UNSTACK)
// =====================================================

void UNomadStatusEffectManagerComponent::RemoveStatusEffect_Implementation(const FGameplayTag EffectTag)
{
    const int32 Index = FindActiveEffectIndexByTag(EffectTag);
    if (Index == INDEX_NONE) return;

    FActiveEffect& Eff = ActiveEffects[Index];
    const int32 PrevStacks = Eff.StackCount;
    const int32 NewStacks = PrevStacks - 1;

    if (Eff.StackCount > 1)
    {
        Eff.StackCount--;
        NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Unstacked, PrevStacks, NewStacks);

        // Inform the effect instance a stack was removed.
        if (Eff.EffectInstance)
        {
            if (UNomadTimedStatusEffect* TimedStatusEffect = Cast<UNomadTimedStatusEffect>(Eff.EffectInstance))
            {
                TimedStatusEffect->OnUnstacked(Eff.StackCount);
            }
        }
    }
    else
    {
        // Last stack: call end logic, destroy instance, remove from array, notify UI.
        if (Eff.EffectInstance)
        {
            Eff.EffectInstance->Nomad_OnStatusEffectEnds();
            Eff.EffectInstance->ConditionalBeginDestroy();
        }
        ActiveEffects.RemoveAt(Index);
        NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Removed, PrevStacks, 0);
    }
}

// =====================================================
//         UI NOTIFICATION
// =====================================================

void UNomadStatusEffectManagerComponent::NotifyAffliction(const FGameplayTag Tag, const ENomadAfflictionNotificationType Type, const int32 PrevStacks, const int32 NewStacks) const
{
    if (const AActor* OwnerActor = GetOwner())
    {
        if (UNomadAfflictionComponent* AfflictionComp = OwnerActor->FindComponentByClass<UNomadAfflictionComponent>())
        {
            AfflictionComp->UpdateAfflictionArray(Tag, Type, PrevStacks, NewStacks);
        }
    }
}
