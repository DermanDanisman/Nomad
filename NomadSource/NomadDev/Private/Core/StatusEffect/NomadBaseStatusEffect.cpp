// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Component/NomadAfflictionComponent.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadBaseStatusEffect::UNomadBaseStatusEffect()
{
    bIsInitialized = false;
    EffectState = EEffectLifecycleState::Removed;
    DamageCauser = nullptr;
}

// =====================================================
//         CONFIGURATION ACCESS & APPLICATION
// =====================================================

UNomadStatusEffectConfigBase* UNomadBaseStatusEffect::GetEffectConfig() const
{
    // Loads the configuration asset for this effect (synchronously).
    if (EffectConfig.IsNull())
    {
        return nullptr;
    }
    return EffectConfig.LoadSynchronous();
}

void UNomadBaseStatusEffect::ApplyBaseConfiguration()
{
    // Loads the config asset and applies all config-driven values (tag, icon, etc).
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
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
    // Returns true if config asset is loaded and valid.
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    return Config && Config->IsConfigValid();
}

// =====================================================
//         STATUS EFFECT PROPERTIES (Category, Tag, Icon)
// =====================================================

ENomadStatusCategory UNomadBaseStatusEffect::GetStatusCategory_Implementation() const
{
    // Returns the status category from the config, or Neutral if not set.
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    if (Config)
    {
        return Config->Category;
    }
    return ENomadStatusCategory::Neutral;
}

void UNomadBaseStatusEffect::ApplyTagFromConfig()
{
    // Applies the effect's gameplay tag from the config to this effect instance.
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    if (Config && Config->EffectTag.IsValid())
    {
        SetStatusEffectTag(Config->EffectTag);
        UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Applied tag from config: %s"), *Config->EffectTag.ToString());
    }
}

void UNomadBaseStatusEffect::ApplyIconFromConfig()
{
    // Applies the effect's icon from the config to this effect instance.
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
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

// =====================================================
//         LIFECYCLE: START / END
// =====================================================

void UNomadBaseStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    // Called when the effect starts on a character (ACF base).
    // Handles config-driven initialization and plays start sound.
    Super::OnStatusEffectStarts_Implementation(Character);
    CharacterOwner = Character;
    EffectState = EEffectLifecycleState::Active;
    InitializeNomadEffect();
    UE_LOG_AFFLICTION(Log, TEXT("[BASE] Enhanced status effect started on %s"), 
                      Character ? *Character->GetName() : TEXT("Unknown"));

    // --- Blocking tags logic (config-driven) ---
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    if (Config && Config->BlockingTags.Num() > 0)
    {
        if (UNomadStatusEffectManagerComponent* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>())
        {
            for (const FGameplayTag& Tag : Config->BlockingTags)
            {
                if (Tag.IsValid())
                {
                    SEManager->AddBlockingTag(Tag);
                }
            }
        }
    }
}

void UNomadBaseStatusEffect::Nomad_OnStatusEffectStarts(ACharacter* Character)
{
    OnStatusEffectStarts_Implementation(Character);
}

void UNomadBaseStatusEffect::Nomad_OnStatusEffectEnds()
{
    // Cleanly ends the effect and transitions state for analytics/cleanup.
    if (EffectState != EEffectLifecycleState::Active)
        return;
    EffectState = EEffectLifecycleState::Ending;
    OnStatusEffectEnds_Implementation();
    EffectState = EEffectLifecycleState::Removed;
}

void UNomadBaseStatusEffect::OnStatusEffectEnds_Implementation()
{
    // Called when the effect is removed from the character (ACF base).
    // Plays end sound, resets init state, and logs for analytics.
    UE_LOG_AFFLICTION(Log, TEXT("[BASE] Enhanced status effect ending"));

    PlayEndSound();

    // --- Remove blocking tags (config-driven) ---
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    if (Config && Config->BlockingTags.Num() > 0 && CharacterOwner)
    {
        if (UNomadStatusEffectManagerComponent* SEManager = CharacterOwner->FindComponentByClass<UNomadStatusEffectManagerComponent>())
        {
            for (const FGameplayTag& Tag : Config->BlockingTags)
            {
                if (Tag.IsValid())
                {
                    SEManager->RemoveBlockingTag(Tag);
                }
            }
        }
    }

    Super::OnStatusEffectEnds_Implementation();

    bIsInitialized = false;
}

// =====================================================
//         INITIALIZATION
// =====================================================

void UNomadBaseStatusEffect::InitializeNomadEffect()
{
    // Applies configuration, plays start sound, and sets the initialized flag.
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

// =====================================================
//         AUDIO/VISUAL HOOKS
// =====================================================

void UNomadBaseStatusEffect::PlayStartSound()
{
    // Loads and plays the start sound, triggers C++ and Blueprint events.
    if (!CharacterOwner) return;
    USoundBase* SoundToPlay = nullptr;
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
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
    // Loads and plays the end sound, triggers C++ and Blueprint events.
    if (!CharacterOwner) return;
    USoundBase* SoundToPlay = nullptr;
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    if (Config && !Config->EndSound.IsNull())
        SoundToPlay = Config->EndSound.LoadSynchronous();
    if (SoundToPlay)
    {
        UGameplayStatics::PlaySoundAtLocation(CharacterOwner->GetWorld(), SoundToPlay, CharacterOwner->GetActorLocation());
        OnEndSoundTriggered_Implementation(SoundToPlay);
        OnEndSoundTriggered(SoundToPlay);
    }
}

void UNomadBaseStatusEffect::ApplySprintBlockTag(ACharacter* Character)
{
    if (!Character) return;
    if (UNomadStatusEffectManagerComponent* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>())
    {
        SEManager->AddBlockingTag(FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Sprint")));
    }
}

void UNomadBaseStatusEffect::RemoveSprintBlockTag(ACharacter* Character)
{
    if (!Character) return;
    if (UNomadStatusEffectManagerComponent* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>())
    {
        SEManager->RemoveBlockingTag(FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Sprint")));
    }
}

// =====================================================
//         INTERNAL HELPERS
// =====================================================

void UNomadBaseStatusEffect::LoadConfigurationValues()
{
    // Applies tag and icon from config asset. (Extend for more config-driven props.)
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    if (!Config) return;
    ApplyTagFromConfig();
    ApplyIconFromConfig();
    UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Configuration values loaded"));
}

// =====================================================
//         HYBRID STAT/DAMAGE APPLICATION (Override in Derived)
// =====================================================

void UNomadBaseStatusEffect::ApplyHybridEffect(const TArray<FStatisticValue>& InStatMods, AActor* InTarget, UObject* InEffectConfig)
{
    // Base implementation does nothing.
    // Child classes must override this to provide hybrid stat/damage/both application logic.
}