// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Component/NomadAfflictionComponent.h"
#include "Core/Debug/NomadLogCategories.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UNomadBaseStatusEffect::UNomadBaseStatusEffect()
{
    bIsInitialized = false;
    EffectState = EEffectLifecycleState::Removed;
    DamageCauser = nullptr;
}

UNomadStatusEffectConfigBase* UNomadBaseStatusEffect::GetBaseConfig() const
{
    if (BaseConfig.IsNull())
    {
        return nullptr;
    }
    return BaseConfig.LoadSynchronous();
}

void UNomadBaseStatusEffect::ApplyBaseConfiguration()
{
    UNomadStatusEffectConfigBase* Config = GetBaseConfig();
    checkf(Config, TEXT("Status Effect Config Data Asset is empty"));

    if (!Config->IsConfigValid())
    {
        UE_LOG_AFFLICTION(Error, TEXT("[BASE] Base configuration validation failed for effect"));
        return;
    }
    LoadConfigurationValues();
    UE_LOG_AFFLICTION(Log, TEXT("[BASE] Base configuration applied: %s"), 
                      *Config->EffectName.ToString());
}

bool UNomadBaseStatusEffect::HasValidBaseConfiguration() const
{
    UNomadStatusEffectConfigBase* Config = GetBaseConfig();
    return Config && Config->IsConfigValid();
}

ENomadStatusCategory UNomadBaseStatusEffect::GetStatusCategory_Implementation() const
{
    UNomadStatusEffectConfigBase* Config = GetBaseConfig();
    if (Config)
    {
        return Config->Category;
    }
    return ENomadStatusCategory::Neutral;
}

void UNomadBaseStatusEffect::ApplyTagFromConfig()
{
    UNomadStatusEffectConfigBase* Config = GetBaseConfig();
    if (Config && Config->EffectTag.IsValid())
    {
        SetStatusEffectTag(Config->EffectTag);
        UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Applied tag from config: %s"), *Config->EffectTag.ToString());
    }
}

void UNomadBaseStatusEffect::ApplyIconFromConfig()
{
    UNomadStatusEffectConfigBase* Config = GetBaseConfig();
    if (Config && !Config->Icon.IsNull())
    {
        UTexture2D* LoadedIcon = Config->Icon.LoadSynchronous();
        if (LoadedIcon)
        {
            SetStatusIcon(LoadedIcon);
            UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Applied icon from config"));
        }
    }
}

void UNomadBaseStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    Super::OnStatusEffectStarts_Implementation(Character);
    EffectState = EEffectLifecycleState::Active;
    InitializeNomadEffect();
    UE_LOG_AFFLICTION(Log, TEXT("[BASE] Enhanced status effect started on %s"), 
                      Character ? *Character->GetName() : TEXT("Unknown"));
}

void UNomadBaseStatusEffect::Nomad_OnStatusEffectEnds()
{
    if (EffectState != EEffectLifecycleState::Active)
        return;
    EffectState = EEffectLifecycleState::Ending;
    OnStatusEffectEnds_Implementation();
    EffectState = EEffectLifecycleState::Removed;
}

void UNomadBaseStatusEffect::OnStatusEffectEnds_Implementation()
{
    UE_LOG_AFFLICTION(Log, TEXT("[BASE] Enhanced status effect ending"));

    PlayEndSound();

    Super::OnStatusEffectEnds_Implementation();

    bIsInitialized = false;
}

void UNomadBaseStatusEffect::InitializeNomadEffect()
{
    if (bIsInitialized)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Effect already initialized"));
        return;
    }
    if (!CharacterOwner)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[BASE] Cannot initialize effect - no character owner"));
        return;
    }
    ApplyBaseConfiguration();
    PlayStartSound();
    bIsInitialized = true;
    UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Nomad effect initialized successfully"));
}

void UNomadBaseStatusEffect::PlayStartSound()
{
    if (!CharacterOwner) return;
    USoundBase* SoundToPlay = nullptr;
    UNomadStatusEffectConfigBase* Config = GetBaseConfig();
    if (Config && !Config->StartSound.IsNull())
        SoundToPlay = Config->StartSound.LoadSynchronous();
    if (SoundToPlay)
    {
        UGameplayStatics::PlaySoundAtLocation(CharacterOwner->GetWorld(), SoundToPlay, CharacterOwner->GetActorLocation());
        OnStartSoundTriggered_Implementation(SoundToPlay);
        OnStartSoundTriggered(SoundToPlay);
    }
}

void UNomadBaseStatusEffect::PlayEndSound()
{
    if (!CharacterOwner) return;
    USoundBase* SoundToPlay = nullptr;
    UNomadStatusEffectConfigBase* Config = GetBaseConfig();
    if (Config && !Config->EndSound.IsNull())
        SoundToPlay = Config->EndSound.LoadSynchronous();
    if (SoundToPlay)
    {
        UGameplayStatics::PlaySoundAtLocation(CharacterOwner->GetWorld(), SoundToPlay, CharacterOwner->GetActorLocation());
        OnEndSoundTriggered_Implementation(SoundToPlay);
        OnEndSoundTriggered(SoundToPlay);
    }
}

void UNomadBaseStatusEffect::LoadConfigurationValues()
{
    UNomadStatusEffectConfigBase* Config = GetBaseConfig();
    if (!Config) return;
    ApplyTagFromConfig();
    ApplyIconFromConfig();
    UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Configuration values loaded"));
}

void UNomadBaseStatusEffect::ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfig)
{
    // Base implementation does nothing.
    // Child classes must override this to provide hybrid stat/damage/both application logic.
}