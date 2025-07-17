// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Component/NomadAfflictionComponent.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "ARSStatisticsComponent.h"
#include "Components/ACFCharacterMovementComponent.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadBaseStatusEffect::UNomadBaseStatusEffect()
{
    // Initialize all runtime state to safe defaults
    bIsInitialized = false;
    EffectState = EEffectLifecycleState::Removed;
    DamageCauser = nullptr;
    
    // Log constructor for debugging memory issues
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[BASE] Status effect constructed"));
}

// =====================================================
//         CONFIGURATION ACCESS & APPLICATION
// =====================================================

UNomadStatusEffectConfigBase* UNomadBaseStatusEffect::GetEffectConfig() const
{
    // Loads the configuration asset for this effect (synchronously).
    // This is safe for runtime use as effects need their config immediately.
    if (EffectConfig.IsNull())
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Effect config is null"));
        return nullptr;
    }
    
    UNomadStatusEffectConfigBase* LoadedConfig = EffectConfig.LoadSynchronous();
    if (!LoadedConfig)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[BASE] Failed to load effect config asset"));
    }
    
    return LoadedConfig;
}

void UNomadBaseStatusEffect::ApplyBaseConfiguration()
{
    // Loads the config asset and applies all config-driven values (tag, icon, etc).
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    if (!Config)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[BASE] Cannot apply configuration - config asset is null"));
        return;
    }

    if (!Config->IsConfigValid())
    {
        UE_LOG_AFFLICTION(Error, TEXT("[BASE] Base configuration validation failed for effect %s"), 
                          *Config->EffectName.ToString());
        return;
    }
    
    // Apply all configuration values
    LoadConfigurationValues();
    
    UE_LOG_AFFLICTION(Log, TEXT("[BASE] Base configuration applied successfully: %s"), 
                      *Config->EffectName.ToString());
}

bool UNomadBaseStatusEffect::HasValidBaseConfiguration() const
{
    // Returns true if config asset is loaded and passes validation.
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    return Config && Config->IsConfigValid();
}

// =====================================================
//         STATUS EFFECT PROPERTIES
// =====================================================

ENomadStatusCategory UNomadBaseStatusEffect::GetStatusCategory_Implementation() const
{
    // Returns the status category from the config, or Neutral if not set.
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    if (Config)
    {
        return Config->Category;
    }
    
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[BASE] No config found, defaulting to Neutral category"));
    return ENomadStatusCategory::Neutral;
}

void UNomadBaseStatusEffect::ApplyTagFromConfig()
{
    // Applies the effect's gameplay tag from the config to this effect instance.
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    if (Config && Config->EffectTag.IsValid())
    {
        SetStatusEffectTag(Config->EffectTag);
        UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Applied tag from config: %s"), 
                          *Config->EffectTag.ToString());
    }
    else
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Cannot apply tag - config missing or tag invalid"));
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
        else
        {
            UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Failed to load icon from config"));
        }
    }
    else
    {
        UE_LOG_AFFLICTION(VeryVerbose, TEXT("[BASE] No icon specified in config"));
    }
}

// =====================================================
//         LIFECYCLE: START / END
// =====================================================

void UNomadBaseStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    // Called when the effect starts on a character (ACF base override).
    // Handles config-driven initialization, blocking tags, and plays start sound.
    
    UE_LOG_AFFLICTION(Log, TEXT("[BASE] Status effect starting on %s"), 
                      Character ? *Character->GetName() : TEXT("Unknown"));
    
    // Call parent implementation first
    Super::OnStatusEffectStarts_Implementation(Character);
    
    // Set our runtime state
    CharacterOwner = Character;
    EffectState = EEffectLifecycleState::Active;
    
    // Initialize the Nomad-specific functionality
    InitializeNomadEffect();

    // Apply blocking tags if configured
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    if (Config && Config->BlockingTags.Num() > 0 && Character)
    {
        if (UNomadStatusEffectManagerComponent* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>())
        {
            for (const FGameplayTag& Tag : Config->BlockingTags)
            {
                if (Tag.IsValid())
                {
                    SEManager->AddBlockingTag(Tag);
                    UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Applied blocking tag: %s"), 
                                      *Tag.ToString());
                }
            }
        }
        else
        {
            UE_LOG_AFFLICTION(Warning, TEXT("[BASE] No status effect manager found for blocking tags"));
        }
    }
    
    UE_LOG_AFFLICTION(Log, TEXT("[BASE] Status effect started successfully"));
}

void UNomadBaseStatusEffect::Nomad_OnStatusEffectStarts(ACharacter* Character)
{
    // Public interface for starting effects - ensures consistent activation
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[BASE] Nomad_OnStatusEffectStarts called"));
    OnStatusEffectStarts_Implementation(Character);
}

void UNomadBaseStatusEffect::Nomad_OnStatusEffectEnds()
{
    // Cleanly ends the effect and transitions state for analytics/cleanup.
    // Prevents double-removal and ensures proper state transitions.
    
    if (EffectState != EEffectLifecycleState::Active)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Attempted to end effect that's not active (state: %d)"), 
                          (int32)EffectState);
        return;
    }
    
    UE_LOG_AFFLICTION(Log, TEXT("[BASE] Ending status effect"));
    
    // Transition to ending state
    EffectState = EEffectLifecycleState::Ending;
    
    // Call the implementation
    OnStatusEffectEnds_Implementation();
    
    // Mark as completely removed
    EffectState = EEffectLifecycleState::Removed;
    
    UE_LOG_AFFLICTION(Log, TEXT("[BASE] Status effect ended successfully"));
}

void UNomadBaseStatusEffect::OnStatusEffectEnds_Implementation()
{
    // Called when the effect is removed from the character (ACF base override).
    // Handles sound playback, blocking tag removal, and cleanup.
    
    UE_LOG_AFFLICTION(Log, TEXT("[BASE] Status effect ending implementation"));

    // Play end sound before any cleanup
    PlayEndSound();

    // Remove blocking tags if they were applied
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
                    UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Removed blocking tag: %s"), 
                                      *Tag.ToString());
                }
            }
        }
    }

    // Call parent implementation
    Super::OnStatusEffectEnds_Implementation();

    // Reset initialization state
    bIsInitialized = false;
    
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[BASE] End implementation completed"));
}

// =====================================================
//         INITIALIZATION
// =====================================================

void UNomadBaseStatusEffect::InitializeNomadEffect()
{
    // Applies configuration, plays start sound, and sets the initialized flag.
    // Prevents double-initialization and ensures proper setup.
    
    if (bIsInitialized)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Effect already initialized, skipping"));
        return;
    }
    
    if (!CharacterOwner)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[BASE] Cannot initialize effect - no character owner"));
        return;
    }
    
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[BASE] Initializing Nomad effect"));
    
    // Apply configuration from data asset
    ApplyBaseConfiguration();
    
    // Play start sound
    PlayStartSound();
    
    // Mark as initialized
    bIsInitialized = true;
    
    UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Nomad effect initialized successfully"));
}

// =====================================================
//         BLOCKING TAG UTILITIES
// =====================================================

void UNomadBaseStatusEffect::ApplySprintBlockTag(ACharacter* Character)
{
    // Utility function to block sprinting while this effect is active.
    // Commonly used by movement-impairing effects.
    
    if (!Character)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Cannot apply sprint block - no character"));
        return;
    }
    
    if (UNomadStatusEffectManagerComponent* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>())
    {
        SEManager->AddBlockingTag(FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Sprint")));
        UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Applied sprint blocking tag"));
    }
    else
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] No status effect manager found for sprint block"));
    }
}

void UNomadBaseStatusEffect::RemoveSprintBlockTag(ACharacter* Character)
{
    // Utility function to remove sprint blocking when effect ends.
    
    if (!Character)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Cannot remove sprint block - no character"));
        return;
    }
    
    if (UNomadStatusEffectManagerComponent* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>())
    {
        SEManager->RemoveBlockingTag(FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Sprint")));
        UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Removed sprint blocking tag"));
    }
    else
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] No status effect manager found for sprint unblock"));
    }
}

void UNomadBaseStatusEffect::ApplyJumpBlockTag(ACharacter* Character)
{
    // Utility function to prevent jumping while this effect is active.
    
    if (!Character)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Cannot apply jump block - no character"));
        return;
    }
    
    if (UNomadStatusEffectManagerComponent* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>())
    {
        SEManager->AddBlockingTag(FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Jump")));
        UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Applied jump blocking tag"));
    }
    else
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] No status effect manager found for jump block"));
    }
}

void UNomadBaseStatusEffect::RemoveJumpBlockTag(ACharacter* Character)
{
    // Utility function to remove jump blocking when effect ends.
    
    if (!Character)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Cannot remove jump block - no character"));
        return;
    }
    
    if (UNomadStatusEffectManagerComponent* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>())
    {
        SEManager->RemoveBlockingTag(FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Jump")));
        UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Removed jump blocking tag"));
    }
    else
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] No status effect manager found for jump unblock"));
    }
}

void UNomadBaseStatusEffect::ApplyMovementSpeedModifier(ACharacter* Character, float Multiplier)
{
    // Applies movement speed modifier through the status effect system.
    // This replaces direct attribute manipulation with config-driven approach.
    
    if (!Character || !GetEffectConfig())
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Cannot apply movement speed modifier - invalid character or config"));
        return;
    }
    
    // The actual movement speed modification should be handled by the status effect's config
    // via PersistentAttributeModifier. This method serves as a hook for additional logic.
    UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Movement speed modifier applied via config (multiplier: %f)"), Multiplier);
    
    // Sync movement speed after applying the status effect
    SyncMovementSpeedFromStatusEffects(Character);
}

void UNomadBaseStatusEffect::RemoveMovementSpeedModifier(ACharacter* Character)
{
    // Removes movement speed modifier applied by this status effect.
    
    if (!Character)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Cannot remove movement speed modifier - no character"));
        return;
    }
    
    UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Movement speed modifier removed via config"));
    
    // Sync movement speed after removing the status effect
    SyncMovementSpeedFromStatusEffects(Character);
}

void UNomadBaseStatusEffect::SyncMovementSpeedFromStatusEffects(ACharacter* Character)
{
    // Syncs movement speed from all active status effect modifiers to ACF movement component.
    // This replaces the hardcoded attribute tag approach with a status effect-driven approach.
    
    if (!Character)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Cannot sync movement speed - no character"));
        return;
    }
    
    const UARSStatisticsComponent* StatsComp = Character->FindComponentByClass<UARSStatisticsComponent>();
    UACFCharacterMovementComponent* MoveComp = Character->FindComponentByClass<UACFCharacterMovementComponent>();
    
    if (!StatsComp || !MoveComp)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Cannot sync movement speed - missing components"));
        return;
    }
    
    // Use the default movement speed attribute tag defined in one place
    static const FGameplayTag DefaultMovementSpeedTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Attributes.MovementSpeed"));
    
    // Get movement speed from attribute system (which includes all status effect modifiers)
    const float NewSpeed = StatsComp->GetCurrentAttributeValue(DefaultMovementSpeedTag);
    
    if (NewSpeed > 0.f)
    {
        MoveComp->MaxWalkSpeed = NewSpeed;
        UE_LOG_AFFLICTION(VeryVerbose, TEXT("[BASE] Synced movement speed to %f"), NewSpeed);
    }
}

// =====================================================
//         AUDIO/VISUAL HOOKS
// =====================================================

void UNomadBaseStatusEffect::PlayStartSound()
{
    // Loads and plays the start sound, triggers C++ and Blueprint events.
    
    if (!CharacterOwner)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Cannot play start sound - no character owner"));
        return;
    }
    
    USoundBase* SoundToPlay = nullptr;
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    
    if (Config && !Config->StartSound.IsNull())
    {
        SoundToPlay = Config->StartSound.LoadSynchronous();
    }
    
    if (SoundToPlay)
    {
        // Play the sound at character location
        UGameplayStatics::PlaySoundAtLocation(
            CharacterOwner->GetWorld(), 
            SoundToPlay, 
            CharacterOwner->GetActorLocation()
        );
        
        // Trigger implementation hooks
        OnStartSoundTriggered_Implementation(SoundToPlay);
        OnStartSoundTriggered(SoundToPlay);
        
        UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Played start sound"));
    }
    else
    {
        UE_LOG_AFFLICTION(VeryVerbose, TEXT("[BASE] No start sound configured"));
    }
}

void UNomadBaseStatusEffect::PlayEndSound()
{
    // Loads and plays the end sound, triggers C++ and Blueprint events.
    
    if (!CharacterOwner)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[BASE] Cannot play end sound - no character owner"));
        return;
    }
    
    USoundBase* SoundToPlay = nullptr;
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    
    if (Config && !Config->EndSound.IsNull())
    {
        SoundToPlay = Config->EndSound.LoadSynchronous();
    }
    
    if (SoundToPlay)
    {
        // Play the sound at character location
        UGameplayStatics::PlaySoundAtLocation(
            CharacterOwner->GetWorld(), 
            SoundToPlay, 
            CharacterOwner->GetActorLocation()
        );
        
        // Trigger implementation hooks
        OnEndSoundTriggered_Implementation(SoundToPlay);
        OnEndSoundTriggered(SoundToPlay);
        
        UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Played end sound"));
    }
    else
    {
        UE_LOG_AFFLICTION(VeryVerbose, TEXT("[BASE] No end sound configured"));
    }
}

// =====================================================
//         INTERNAL HELPERS
// =====================================================

void UNomadBaseStatusEffect::LoadConfigurationValues()
{
    // Applies tag and icon from config asset.
    // Can be extended for additional config-driven properties.
    
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    if (!Config)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[BASE] Cannot load configuration values - config is null"));
        return;
    }
    
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[BASE] Loading configuration values"));
    
    // Apply gameplay tag
    ApplyTagFromConfig();
    
    // Apply icon
    ApplyIconFromConfig();
    
    UE_LOG_AFFLICTION(Verbose, TEXT("[BASE] Configuration values loaded successfully"));
}

// =====================================================
//         HYBRID STAT/DAMAGE APPLICATION
// =====================================================

void UNomadBaseStatusEffect::ApplyHybridEffect(const TArray<FStatisticValue>& InStatMods, AActor* InTarget, UObject* InEffectConfig)
{
    // Base implementation does nothing - child classes must override this.
    // This allows for polymorphic behavior while keeping the base class abstract.
    
    UE_LOG_AFFLICTION(Warning, TEXT("[BASE] ApplyHybridEffect called on base class - override in derived classes"));
    
    // Log the parameters for debugging
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[BASE] Hybrid effect called with %d stat mods, target: %s"), 
                      InStatMods.Num(), InTarget ? *InTarget->GetName() : TEXT("null"));
}