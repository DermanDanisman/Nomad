# Nomad Survival System - Developer Implementation Guide

**Version:** 1.0  
**Last Updated:** 2025-01-27  
**Audience:** Programmers, Technical Designers, System Architects  

---

## Table of Contents

1. [Code Architecture Deep Dive](#code-architecture-deep-dive)
2. [API Documentation](#api-documentation)
3. [Extension and Customization](#extension-and-customization)
4. [System Integration Patterns](#system-integration-patterns)
5. [Performance Optimization](#performance-optimization)
6. [Debugging and Testing Tools](#debugging-and-testing-tools)
7. [Migration Patterns](#migration-patterns)
8. [Advanced Implementation Examples](#advanced-implementation-examples)

---

## Code Architecture Deep Dive

### Core Class Hierarchy

#### UNomadBaseStatusEffect
**Location**: `NomadSource/NomadDev/Public/Core/StatusEffect/NomadBaseStatusEffect.h`

```cpp
/**
 * Abstract base class for all Nomad status effects.
 * Extends ACF's UACFBaseStatusEffect with Nomad-specific functionality.
 */
class NOMADDEV_API UNomadBaseStatusEffect : public UACFBaseStatusEffect
{
    GENERATED_BODY()

public:
    // Lifecycle Management
    virtual void OnStatusEffectStarts_Implementation() override;
    virtual void OnStatusEffectEnds_Implementation() override;
    virtual bool CanBeApplied_Implementation() override;
    
    // Configuration System
    UFUNCTION(BlueprintCallable, Category="Status Effect")
    UNomadStatusEffectConfigBase* GetEffectConfig() const;
    
    void ApplyBaseConfiguration();
    bool HasValidBaseConfiguration() const;
    
    // Category System for UI
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Status Effect")
    ENomadStatusCategory GetStatusCategory() const;
    
    // Blocking System for Input Restrictions
    UFUNCTION(BlueprintCallable, Category="Status Effect")
    void ApplyJumpBlockTag(ACharacter* Character);
    
    UFUNCTION(BlueprintCallable, Category="Status Effect")
    void RemoveJumpBlockTag(ACharacter* Character);
    
    // Movement Speed Integration
    UFUNCTION(BlueprintCallable, Category="Status Effect")
    void SyncMovementSpeedModifier(ACharacter* Character, float Multiplier);

protected:
    // Configuration Asset Reference
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Configuration")
    TSoftObjectPtr<UNomadStatusEffectConfigBase> EffectConfig;
    
    // Runtime State
    UPROPERTY(BlueprintReadOnly, Category="Status Effect")
    EEffectLifecycleState EffectState;
    
    UPROPERTY(BlueprintReadOnly, Category="Status Effect")
    bool bIsInitialized;
    
    // Audio/Visual Integration
    virtual void PlayConfiguredVisualEffect(EVisualEffectType EffectType);
    virtual void PlayConfiguredAudioCue(EAudioCueType CueType);
};
```

#### UNomadSurvivalStatusEffect
**Location**: `NomadSource/NomadDev/Public/Core/StatusEffect/SurvivalHazard/NomadSurvivalStatusEffect.h`

```cpp
/**
 * Specialized status effect for survival mechanics.
 * Inherits from UNomadInfiniteStatusEffect for persistent effects.
 */
class NOMADDEV_API UNomadSurvivalStatusEffect : public UNomadInfiniteStatusEffect
{
    GENERATED_BODY()

public:
    UNomadSurvivalStatusEffect();
    
    // Severity Management
    UFUNCTION(BlueprintCallable, Category="Survival Effect")
    void SetSeverityLevel(ESurvivalSeverity InSeverity);
    
    UFUNCTION(BlueprintPure, Category="Survival Effect")
    ESurvivalSeverity GetSeverityLevel() const;
    
    // Damage Over Time Implementation
    virtual void HandleInfiniteTick_Implementation(float DeltaTime) override;
    
protected:
    // Current severity level for this effect instance
    UPROPERTY(BlueprintReadOnly, Category="Survival Effect")
    ESurvivalSeverity CurrentSeverity;
    
    // Cached hazard configuration for performance
    UPROPERTY(Transient)
    const FNomadHazardConfigRow* CachedHazardConfig;
    
    // DoT Application Methods
    void ApplyDamageOverTime(float DeltaTime);
    void CalculateDamageAmount(float DeltaTime, float& OutDamage) const;
    
    // Severity-based Effect Scaling
    float GetSeverityMultiplier() const;
    void UpdateEffectIntensityForSeverity();
};
```

### Component Architecture

#### UNomadSurvivalNeedsComponent
**Location**: `NomadSource/NomadDev/Public/Core/Component/NomadSurvivalNeedsComponent.h`

```cpp
/**
 * Central component managing survival mechanics (hunger, thirst, temperature).
 * Integrates with status effect system for condition-based effects.
 */
class NOMADDEV_API UNomadSurvivalNeedsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNomadSurvivalNeedsComponent();
    
    // Component Lifecycle
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
                              FActorComponentTickFunction* ThisTickFunction) override;
    
    // Stat Management API
    UFUNCTION(BlueprintCallable, Category="Survival")
    void ModifyHunger(float Amount);
    
    UFUNCTION(BlueprintCallable, Category="Survival")
    void ModifyThirst(float Amount);
    
    UFUNCTION(BlueprintCallable, Category="Survival")
    void ModifyBodyTemperature(float Amount);
    
    // Status Effect Integration
    UFUNCTION(BlueprintCallable, Category="Survival")
    void ApplySurvivalStatusEffect(ESurvivalSeverity Severity, FGameplayTag EffectTag);
    
    UFUNCTION(BlueprintCallable, Category="Survival")
    void RemoveSurvivalStatusEffect(FGameplayTag EffectTag);
    
    // Configuration and Data Access
    UFUNCTION(BlueprintCallable, Category="Survival")
    void LoadSurvivalConfig();
    
    UFUNCTION(BlueprintPure, Category="Survival")
    float GetHungerLevel() const { return CurrentHunger; }
    
    UFUNCTION(BlueprintPure, Category="Survival")
    float GetThirstLevel() const { return CurrentThirst; }

protected:
    // Survival Stats (Replicated)
    UPROPERTY(ReplicatedUsing=OnRep_HungerLevel, BlueprintReadOnly, Category="Survival")
    float CurrentHunger;
    
    UPROPERTY(ReplicatedUsing=OnRep_ThirstLevel, BlueprintReadOnly, Category="Survival")
    float CurrentThirst;
    
    UPROPERTY(ReplicatedUsing=OnRep_BodyTemperature, BlueprintReadOnly, Category="Survival")
    float CurrentBodyTemperature;
    
    // Configuration
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Configuration")
    TSoftObjectPtr<UNomadSurvivalNeedsData> SurvivalConfig;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Configuration")
    TSoftObjectPtr<UNomadSurvivalHazardConfig> HazardConfig;
    
    // Internal Tick Logic
    void ProcessSurvivalDecay(float DeltaTime);
    void CheckSurvivalThresholds();
    void UpdateSurvivalEffects();
    
    // Replication Callbacks
    UFUNCTION()
    void OnRep_HungerLevel();
    
    UFUNCTION()
    void OnRep_ThirstLevel();
    
    UFUNCTION()
    void OnRep_BodyTemperature();
    
    // Component References (Cached)
    UPROPERTY(Transient)
    UNomadStatusEffectManagerComponent* StatusEffectManager;
    
    UPROPERTY(Transient)
    UARSStatisticsComponent* StatisticsComponent;
};
```

---

## API Documentation

### Core Status Effect Management

#### Applying Status Effects

```cpp
// Basic application through manager component
UNomadStatusEffectManagerComponent* SEManager = Character->GetComponentByClass<UNomadStatusEffectManagerComponent>();
bool bSuccess = SEManager->ApplyStatusEffect(UNomadTimedStatusEffect::StaticClass());

// Advanced application with custom configuration
FStatusEffectApplicationData AppData;
AppData.EffectClass = UNomadSurvivalStatusEffect::StaticClass();
AppData.EffectTag = FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Starvation");
AppData.Duration = -1.0f; // Infinite
AppData.Severity = ESurvivalSeverity::Severe;
bool bApplied = SEManager->ApplyStatusEffectAdvanced(AppData);

// Blueprint-accessible wrapper
UFUNCTION(BlueprintCallable, Category="Status Effects")
static bool ApplyStatusEffectByTag(ACharacter* Character, FGameplayTag EffectTag, float Duration = -1.0f);
```

#### Querying Active Effects

```cpp
// Check if specific effect is active
bool bHasEffect = SEManager->HasActiveEffect(FGameplayTag::RequestGameplayTag("StatusEffect.Condition.Bleeding"));

// Get all active effects
TArray<FNomadStatusEffect> ActiveEffects = SEManager->GetActiveEffects();

// Get effects by category
TArray<FNomadStatusEffect> NegativeEffects = SEManager->GetActiveEffectsByCategory(ENomadStatusCategory::Negative);

// Check effect stacks
int32 BleedingStacks = SEManager->GetEffectStackCount(FGameplayTag::RequestGameplayTag("StatusEffect.Condition.Bleeding"));
```

#### Removing Status Effects

```cpp
// Remove specific effect
bool bRemoved = SEManager->RemoveStatusEffect(FGameplayTag::RequestGameplayTag("StatusEffect.Potion.SpeedBoost"));

// Remove all effects of a category
int32 RemovedCount = SEManager->RemoveEffectsByCategory(ENomadStatusCategory::Negative);

// Remove all effects (emergency cleanup)
SEManager->RemoveAllStatusEffects();

// Smart removal with conditions
SEManager->RemoveEffectsIf([](const FNomadStatusEffect& Effect) {
    return Effect.GetDuration() < 5.0f; // Remove effects with less than 5 seconds left
});
```

### Configuration Asset API

#### Creating Configuration Assets Programmatically

```cpp
// Create new effect configuration at runtime
UNomadTimedEffectConfig* CreatePoisonEffect()
{
    UNomadTimedEffectConfig* Config = NewObject<UNomadTimedEffectConfig>();
    
    // Basic information
    Config->EffectName = FText::FromString("Poison");
    Config->Description = FText::FromString("Loses health over time from toxins");
    Config->EffectTag = FGameplayTag::RequestGameplayTag("StatusEffect.Condition.Poison");
    Config->Category = ENomadStatusCategory::Negative;
    
    // Timing configuration
    Config->Duration = 15.0f;
    Config->TickInterval = 1.0f;
    
    // Damage configuration
    Config->ApplicationMode = EStatusEffectApplicationMode::DamageEvent;
    Config->DamageTypeClass = UPoisonDamageType::StaticClass();
    Config->StatModifiersPerTick.Add("RPG.Statistics.Health", -3.0f);
    
    // Stacking configuration
    Config->CanStack = true;
    Config->MaxStacks = 3;
    
    return Config;
}
```

#### Configuration Validation

```cpp
// Validate configuration before use
bool ValidateEffectConfig(UNomadStatusEffectConfigBase* Config)
{
    if (!Config)
    {
        UE_LOG(LogNomadAffliction, Error, TEXT("Config is null"));
        return false;
    }
    
    // Use built-in validation
    if (!Config->IsConfigValid())
    {
        UE_LOG(LogNomadAffliction, Error, TEXT("Config validation failed: %s"), *Config->GetName());
        return false;
    }
    
    // Custom validation logic
    if (Config->ApplicationMode == EStatusEffectApplicationMode::DamageEvent && !Config->DamageTypeClass)
    {
        UE_LOG(LogNomadAffliction, Error, TEXT("DamageEvent mode requires DamageTypeClass"));
        return false;
    }
    
    return true;
}
```

### Survival System API

#### Survival Stat Management

```cpp
// Direct stat modification
void ModifySurvivalStats(UNomadSurvivalNeedsComponent* SurvivalComp)
{
    // Immediate changes
    SurvivalComp->ModifyHunger(-10.0f);    // Reduce hunger by 10
    SurvivalComp->ModifyThirst(-15.0f);    // Reduce thirst by 15
    SurvivalComp->ModifyBodyTemperature(5.0f); // Increase temperature by 5
    
    // Clamp values to valid ranges
    SurvivalComp->ClampSurvivalStats();
    
    // Force immediate threshold check
    SurvivalComp->CheckSurvivalThresholds();
}

// Rate-based modification (for environmental effects)
void ApplyEnvironmentalEffect(UNomadSurvivalNeedsComponent* SurvivalComp, float DeltaTime)
{
    // Get environmental multipliers
    float TemperatureMultiplier = GetEnvironmentTemperatureMultiplier();
    float HumidityMultiplier = GetEnvironmentHumidityMultiplier();
    
    // Apply environmental decay
    float ThirstDecay = SurvivalComp->GetBaseThirstDecayRate() * TemperatureMultiplier * DeltaTime;
    float TemperatureChange = GetEnvironmentTemperatureDelta() * DeltaTime;
    
    SurvivalComp->ModifyThirst(-ThirstDecay);
    SurvivalComp->ModifyBodyTemperature(TemperatureChange);
}
```

#### Custom Survival Effects

```cpp
// Create custom survival effect class
UCLASS(BlueprintType)
class NOMADDEV_API UNomadCustomSurvivalEffect : public UNomadSurvivalStatusEffect
{
    GENERATED_BODY()

public:
    UNomadCustomSurvivalEffect();

protected:
    // Override tick behavior for custom logic
    virtual void HandleInfiniteTick_Implementation(float DeltaTime) override
    {
        Super::HandleInfiniteTick_Implementation(DeltaTime);
        
        // Custom behavior based on time of day
        if (IsNightTime())
        {
            ApplyNightTimeEffects(DeltaTime);
        }
        
        // Custom behavior based on player activity
        if (IsPlayerRunning())
        {
            ApplyExertionEffects(DeltaTime);
        }
    }
    
    // Custom severity calculation
    virtual float GetSeverityMultiplier() const override
    {
        float BaseMult = Super::GetSeverityMultiplier();
        
        // Custom multiplier based on equipment
        if (HasWarmClothing())
        {
            BaseMult *= 0.5f; // Reduce cold effects with warm clothing
        }
        
        return BaseMult;
    }

private:
    void ApplyNightTimeEffects(float DeltaTime);
    void ApplyExertionEffects(float DeltaTime);
    bool IsNightTime() const;
    bool IsPlayerRunning() const;
    bool HasWarmClothing() const;
};
```

---

## Extension and Customization

### Creating Custom Status Effect Types

#### Custom Instant Effect

```cpp
UCLASS(BlueprintType)
class NOMADDEV_API UNomadHealingPotionEffect : public UNomadInstantStatusEffect
{
    GENERATED_BODY()

public:
    UNomadHealingPotionEffect();

protected:
    virtual void OnStatusEffectStarts_Implementation() override
    {
        Super::OnStatusEffectStarts_Implementation();
        
        // Get healing amount from configuration
        const UNomadInstantEffectConfig* Config = Cast<UNomadInstantEffectConfig>(GetEffectConfig());
        if (!Config) return;
        
        // Apply healing with visual/audio feedback
        ApplyInstantHealing(Config);
        PlayHealingEffects();
        
        // Check for bonus effects based on player state
        CheckForBonusEffects();
    }

private:
    void ApplyInstantHealing(const UNomadInstantEffectConfig* Config)
    {
        ACharacter* Character = GetTargetCharacter();
        if (!Character) return;
        
        // Get healing amount from config
        float HealAmount = Config->StatModifiers.FindRef("RPG.Statistics.Health");
        
        // Apply scaling based on character level/stats
        float ScaledHealAmount = HealAmount * GetHealingScaleMultiplier(Character);
        
        // Apply through attribute system
        UARSStatisticsComponent* Stats = Character->GetComponentByClass<UARSStatisticsComponent>();
        if (Stats)
        {
            Stats->ModifyStatistic(FGameplayTag::RequestGameplayTag("RPG.Statistics.Health"), ScaledHealAmount);
        }
    }
    
    void PlayHealingEffects()
    {
        // Custom visual effects
        if (HealingParticleEffect)
        {
            UGameplayStatics::SpawnEmitterAttached(HealingParticleEffect, GetTargetCharacter()->GetRootComponent());
        }
        
        // Custom audio
        if (HealingSoundCue)
        {
            UGameplayStatics::PlaySoundAtLocation(this, HealingSoundCue, GetTargetCharacter()->GetActorLocation());
        }
    }
    
    void CheckForBonusEffects()
    {
        ACharacter* Character = GetTargetCharacter();
        if (!Character) return;
        
        // Bonus effect if player is in combat
        if (IsCharacterInCombat(Character))
        {
            // Apply temporary damage resistance
            UNomadStatusEffectManagerComponent* SEManager = Character->GetComponentByClass<UNomadStatusEffectManagerComponent>();
            SEManager->ApplyStatusEffect(UNomadCombatResistanceEffect::StaticClass());
        }
    }
    
    float GetHealingScaleMultiplier(ACharacter* Character) const
    {
        // Scale healing based on character's medical skill, equipment, etc.
        float Multiplier = 1.0f;
        
        // Example: Check for medical training
        if (HasMedicalTraining(Character))
        {
            Multiplier *= 1.2f;
        }
        
        // Example: Check for first aid kit
        if (HasFirstAidKit(Character))
        {
            Multiplier *= 1.1f;
        }
        
        return Multiplier;
    }
    
    UPROPERTY(EditDefaultsOnly, Category="Effects")
    UParticleSystem* HealingParticleEffect;
    
    UPROPERTY(EditDefaultsOnly, Category="Effects")
    USoundCue* HealingSoundCue;
};
```

#### Custom Timed Effect with Dynamic Behavior

```cpp
UCLASS(BlueprintType)
class NOMADDEV_API UNomadAdaptiveBuffEffect : public UNomadTimedStatusEffect
{
    GENERATED_BODY()

public:
    UNomadAdaptiveBuffEffect();

protected:
    virtual void HandleTimedTick_Implementation(float DeltaTime) override
    {
        Super::HandleTimedTick_Implementation(DeltaTime);
        
        // Adaptive behavior based on combat state
        UpdateEffectBasedOnCombatState();
        
        // Dynamic intensity based on effect duration
        UpdateEffectIntensity();
    }

private:
    void UpdateEffectBasedOnCombatState()
    {
        ACharacter* Character = GetTargetCharacter();
        if (!Character) return;
        
        bool bInCombat = IsCharacterInCombat(Character);
        
        if (bInCombat != bWasInCombat)
        {
            if (bInCombat)
            {
                // Enhance effect when entering combat
                ApplyCombatBonus();
            }
            else
            {
                // Reduce effect when leaving combat
                RemoveCombatBonus();
            }
            
            bWasInCombat = bInCombat;
        }
    }
    
    void UpdateEffectIntensity()
    {
        float RemainingDuration = GetRemainingDuration();
        float TotalDuration = GetTotalDuration();
        float TimeRatio = RemainingDuration / TotalDuration;
        
        // Effect gets stronger as it nears expiration (last hurrah)
        if (TimeRatio < 0.2f) // Last 20% of duration
        {
            if (!bFinalBurstApplied)
            {
                ApplyFinalBurst();
                bFinalBurstApplied = true;
            }
        }
    }
    
    void ApplyCombatBonus()
    {
        // Temporarily increase attack damage
        UARSStatisticsComponent* Stats = GetTargetCharacter()->GetComponentByClass<UARSStatisticsComponent>();
        if (Stats)
        {
            CombatBonusGuid = FGuid::NewGuid();
            Stats->AddAttributeModifier(
                FGameplayTag::RequestGameplayTag("RPG.Attributes.AttackDamage"),
                1.5f, // 50% bonus damage in combat
                CombatBonusGuid
            );
        }
    }
    
    void RemoveCombatBonus()
    {
        if (CombatBonusGuid.IsValid())
        {
            UARSStatisticsComponent* Stats = GetTargetCharacter()->GetComponentByClass<UARSStatisticsComponent>();
            if (Stats)
            {
                Stats->RemoveAttributeModifier(
                    FGameplayTag::RequestGameplayTag("RPG.Attributes.AttackDamage"),
                    CombatBonusGuid
                );
            }
            CombatBonusGuid.Invalidate();
        }
    }
    
    void ApplyFinalBurst()
    {
        // Apply temporary invincibility frames
        ACharacter* Character = GetTargetCharacter();
        if (Character)
        {
            // Custom invincibility logic
            Character->SetActorEnableCollision(false);
            
            // Set timer to restore collision
            FTimerHandle TimerHandle;
            Character->GetWorldTimerManager().SetTimer(TimerHandle, [Character]()
            {
                if (IsValid(Character))
                {
                    Character->SetActorEnableCollision(true);
                }
            }, 2.0f, false);
        }
    }
    
    UPROPERTY(Transient)
    bool bWasInCombat = false;
    
    UPROPERTY(Transient)
    bool bFinalBurstApplied = false;
    
    UPROPERTY(Transient)
    FGuid CombatBonusGuid;
};
```

### Custom Configuration Assets

#### Advanced Configuration with Validation

```cpp
UCLASS(BlueprintType)
class NOMADDEV_API UNomadAdvancedEffectConfig : public UNomadTimedEffectConfig
{
    GENERATED_BODY()

public:
    UNomadAdvancedEffectConfig();
    
    // Advanced validation
    virtual bool IsConfigValid() const override
    {
        if (!Super::IsConfigValid())
        {
            return false;
        }
        
        // Custom validation rules
        if (AdaptiveBehavior.bEnabled && AdaptiveBehavior.Conditions.Num() == 0)
        {
            UE_LOG(LogNomadAffliction, Error, TEXT("Adaptive behavior enabled but no conditions specified"));
            return false;
        }
        
        if (ScalingFactors.bEnabled && ScalingFactors.MaxScaleMultiplier <= ScalingFactors.MinScaleMultiplier)
        {
            UE_LOG(LogNomadAffliction, Error, TEXT("Invalid scaling factor range"));
            return false;
        }
        
        return true;
    }

    // Advanced configuration options
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Advanced Behavior")
    FAdaptiveBehaviorConfig AdaptiveBehavior;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Advanced Behavior")
    FScalingFactorsConfig ScalingFactors;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Advanced Behavior")
    FConditionalEffectsConfig ConditionalEffects;
};

// Supporting structures
USTRUCT(BlueprintType)
struct FAdaptiveBehaviorConfig
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bEnabled = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="bEnabled"))
    TArray<FGameplayTag> Conditions;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="bEnabled"))
    float AdaptationRate = 1.0f;
};

USTRUCT(BlueprintType)
struct FScalingFactorsConfig
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bEnabled = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="bEnabled"))
    float MinScaleMultiplier = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="bEnabled"))
    float MaxScaleMultiplier = 2.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="bEnabled"))
    FGameplayTag ScalingAttribute;
};
```

---

## System Integration Patterns

### Integration with Equipment System

```cpp
// Equipment-based status effect application
class NOMADDEV_API UNomadEquipmentStatusEffectInterface
{
public:
    // Apply equipment effects when item is equipped
    static void ApplyEquipmentEffects(ACharacter* Character, const FEquipmentItemData& ItemData)
    {
        UNomadStatusEffectManagerComponent* SEManager = Character->GetComponentByClass<UNomadStatusEffectManagerComponent>();
        if (!SEManager) return;
        
        // Apply stat modifiers from equipment
        for (const FEquipmentStatusEffect& EquipEffect : ItemData.StatusEffects)
        {
            FStatusEffectApplicationData AppData;
            AppData.EffectClass = EquipEffect.EffectClass;
            AppData.EffectTag = EquipEffect.EffectTag;
            AppData.Duration = -1.0f; // Infinite while equipped
            AppData.SourceGuid = ItemData.ItemGuid; // Track source for removal
            
            SEManager->ApplyStatusEffectAdvanced(AppData);
        }
    }
    
    // Remove equipment effects when item is unequipped
    static void RemoveEquipmentEffects(ACharacter* Character, const FEquipmentItemData& ItemData)
    {
        UNomadStatusEffectManagerComponent* SEManager = Character->GetComponentByClass<UNomadStatusEffectManagerComponent>();
        if (!SEManager) return;
        
        // Remove effects by source GUID
        SEManager->RemoveEffectsBySource(ItemData.ItemGuid);
    }
};

// Usage in equipment system
void UEquipmentComponent::EquipItem(const FEquipmentItemData& ItemData)
{
    // Standard equipment logic...
    PerformEquipmentLogic(ItemData);
    
    // Apply status effects
    UNomadEquipmentStatusEffectInterface::ApplyEquipmentEffects(GetOwner<ACharacter>(), ItemData);
    
    // Notify other systems
    OnItemEquipped.Broadcast(ItemData);
}
```

### Integration with Damage System

```cpp
// Custom damage type that applies status effects
UCLASS(BlueprintType)
class NOMADDEV_API UNomadStatusEffectDamageType : public UDamageType
{
    GENERATED_BODY()

public:
    // Status effects to apply when this damage type is dealt
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Status Effects")
    TArray<FStatusEffectOnDamage> StatusEffectsOnDamage;
    
    // Chance-based status effect application
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Status Effects")
    TArray<FChanceBasedStatusEffect> ChanceBasedEffects;
};

USTRUCT(BlueprintType)
struct FStatusEffectOnDamage
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<UNomadBaseStatusEffect> EffectClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag EffectTag;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float MinDamageThreshold = 0.0f; // Only apply if damage >= threshold
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bScaleWithDamage = false; // Scale effect intensity with damage amount
};

// Integration in damage processing
void UNomadDamageProcessor::ProcessDamageEvent(const FDamageEvent& DamageEvent, ACharacter* DamagedCharacter)
{
    // Standard damage processing...
    float FinalDamage = CalculateFinalDamage(DamageEvent, DamagedCharacter);
    
    // Apply status effects from damage type
    if (const UNomadStatusEffectDamageType* StatusDamageType = Cast<UNomadStatusEffectDamageType>(DamageEvent.DamageTypeClass))
    {
        ApplyDamageStatusEffects(StatusDamageType, DamagedCharacter, FinalDamage);
    }
    
    // Apply final damage
    ApplyDamageToCharacter(DamagedCharacter, FinalDamage);
}

void UNomadDamageProcessor::ApplyDamageStatusEffects(const UNomadStatusEffectDamageType* DamageType, 
                                                     ACharacter* Character, float DamageAmount)
{
    UNomadStatusEffectManagerComponent* SEManager = Character->GetComponentByClass<UNomadStatusEffectManagerComponent>();
    if (!SEManager) return;
    
    for (const FStatusEffectOnDamage& EffectData : DamageType->StatusEffectsOnDamage)
    {
        // Check damage threshold
        if (DamageAmount < EffectData.MinDamageThreshold) continue;
        
        // Apply status effect
        FStatusEffectApplicationData AppData;
        AppData.EffectClass = EffectData.EffectClass;
        AppData.EffectTag = EffectData.EffectTag;
        
        // Scale with damage if enabled
        if (EffectData.bScaleWithDamage)
        {
            AppData.IntensityMultiplier = FMath::Clamp(DamageAmount / 100.0f, 0.1f, 5.0f);
        }
        
        SEManager->ApplyStatusEffectAdvanced(AppData);
    }
}
```

### Integration with Save/Load System

```cpp
// Serializable status effect data
USTRUCT(BlueprintType)
struct FNomadStatusEffectSaveData
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite)
    FGameplayTag EffectTag;
    
    UPROPERTY(BlueprintReadWrite)
    TSubclassOf<UNomadBaseStatusEffect> EffectClass;
    
    UPROPERTY(BlueprintReadWrite)
    float RemainingDuration;
    
    UPROPERTY(BlueprintReadWrite)
    int32 StackCount;
    
    UPROPERTY(BlueprintReadWrite)
    FGuid SourceGuid;
    
    UPROPERTY(BlueprintReadWrite)
    float IntensityMultiplier;
    
    // Custom data for effect-specific state
    UPROPERTY(BlueprintReadWrite)
    TMap<FString, FString> CustomData;
};

// Save system integration
class NOMADDEV_API UNomadStatusEffectSaveInterface
{
public:
    // Save all active status effects
    static TArray<FNomadStatusEffectSaveData> SaveStatusEffects(ACharacter* Character)
    {
        TArray<FNomadStatusEffectSaveData> SaveData;
        
        UNomadStatusEffectManagerComponent* SEManager = Character->GetComponentByClass<UNomadStatusEffectManagerComponent>();
        if (!SEManager) return SaveData;
        
        TArray<FNomadStatusEffect> ActiveEffects = SEManager->GetActiveEffects();
        for (const FNomadStatusEffect& Effect : ActiveEffects)
        {
            FNomadStatusEffectSaveData EffectSaveData;
            EffectSaveData.EffectTag = Effect.GetEffectTag();
            EffectSaveData.EffectClass = Effect.GetEffectClass();
            EffectSaveData.RemainingDuration = Effect.GetRemainingDuration();
            EffectSaveData.StackCount = Effect.GetStackCount();
            EffectSaveData.SourceGuid = Effect.GetSourceGuid();
            EffectSaveData.IntensityMultiplier = Effect.GetIntensityMultiplier();
            
            // Allow effects to save custom data
            if (UNomadBaseStatusEffect* EffectInstance = Effect.GetEffectInstance())
            {
                EffectInstance->SaveCustomData(EffectSaveData.CustomData);
            }
            
            SaveData.Add(EffectSaveData);
        }
        
        return SaveData;
    }
    
    // Restore status effects from save data
    static void LoadStatusEffects(ACharacter* Character, const TArray<FNomadStatusEffectSaveData>& SaveData)
    {
        UNomadStatusEffectManagerComponent* SEManager = Character->GetComponentByClass<UNomadStatusEffectManagerComponent>();
        if (!SEManager) return;
        
        // Clear existing effects
        SEManager->RemoveAllStatusEffects();
        
        // Restore saved effects
        for (const FNomadStatusEffectSaveData& EffectData : SaveData)
        {
            FStatusEffectApplicationData AppData;
            AppData.EffectClass = EffectData.EffectClass;
            AppData.EffectTag = EffectData.EffectTag;
            AppData.Duration = EffectData.RemainingDuration;
            AppData.StackCount = EffectData.StackCount;
            AppData.SourceGuid = EffectData.SourceGuid;
            AppData.IntensityMultiplier = EffectData.IntensityMultiplier;
            
            if (SEManager->ApplyStatusEffectAdvanced(AppData))
            {
                // Restore custom data
                if (UNomadBaseStatusEffect* EffectInstance = SEManager->GetActiveEffectInstance(EffectData.EffectTag))
                {
                    EffectInstance->LoadCustomData(EffectData.CustomData);
                }
            }
        }
    }
};
```

---

## Performance Optimization

### Tick Management and Batching

```cpp
// Optimized tick manager for status effects
class NOMADDEV_API UNomadStatusEffectTickManager : public UObject
{
    GENERATED_BODY()

public:
    // Singleton instance
    static UNomadStatusEffectTickManager* Get();
    
    // Register effect for optimized ticking
    void RegisterEffect(UNomadBaseStatusEffect* Effect, float TickInterval);
    void UnregisterEffect(UNomadBaseStatusEffect* Effect);
    
    // Tick management
    void TickManager(float DeltaTime);

private:
    // Tick groups for batching similar effects
    struct FTickGroup
    {
        float TickInterval;
        float NextTickTime;
        TArray<TWeakObjectPtr<UNomadBaseStatusEffect>> Effects;
    };
    
    // Organized by tick interval for efficient batching
    TMap<float, FTickGroup> TickGroups;
    
    // Performance monitoring
    int32 EffectsTickedThisFrame = 0;
    float MaxTickTimePerFrame = 2.0f; // 2ms budget per frame
    
    void ProcessTickGroup(FTickGroup& Group, float DeltaTime);
    void OptimizeTickGroups(); // Reorganize for better performance
};

// Usage in status effects
void UNomadBaseStatusEffect::BeginPlay()
{
    Super::BeginPlay();
    
    // Register with optimized tick manager instead of using UE tick
    const UNomadStatusEffectConfigBase* Config = GetEffectConfig();
    float TickInterval = Config ? Config->GetTickInterval() : 1.0f;
    
    UNomadStatusEffectTickManager::Get()->RegisterEffect(this, TickInterval);
}
```

### Memory Pool Management

```cpp
// Object pool for status effects
class NOMADDEV_API UNomadStatusEffectPool : public UObject
{
    GENERATED_BODY()

public:
    // Get pooled effect instance
    template<typename T>
    T* GetPooledEffect()
    {
        TSubclassOf<UNomadBaseStatusEffect> EffectClass = T::StaticClass();
        return Cast<T>(GetPooledEffectByClass(EffectClass));
    }
    
    UNomadBaseStatusEffect* GetPooledEffectByClass(TSubclassOf<UNomadBaseStatusEffect> EffectClass);
    
    // Return effect to pool
    void ReturnEffectToPool(UNomadBaseStatusEffect* Effect);
    
    // Pool management
    void PrewarmPool(TSubclassOf<UNomadBaseStatusEffect> EffectClass, int32 Count);
    void CleanupPool(); // Remove unused objects

private:
    // Pool storage by class type
    TMap<TSubclassOf<UNomadBaseStatusEffect>, TArray<UNomadBaseStatusEffect*>> AvailablePools;
    TArray<UNomadBaseStatusEffect*> ActiveEffects;
    
    // Pool configuration
    static constexpr int32 MaxPoolSizePerClass = 50;
    static constexpr int32 PrewarmSizePerClass = 10;
    
    void CreatePooledEffect(TSubclassOf<UNomadBaseStatusEffect> EffectClass);
};

// Integration with status effect manager
UNomadBaseStatusEffect* UNomadStatusEffectManagerComponent::CreateStatusEffect(TSubclassOf<UNomadBaseStatusEffect> EffectClass)
{
    // Use pool instead of creating new objects
    UNomadBaseStatusEffect* Effect = UNomadStatusEffectPool::Get()->GetPooledEffectByClass(EffectClass);
    
    if (!Effect)
    {
        // Fallback to normal creation if pool is exhausted
        Effect = NewObject<UNomadBaseStatusEffect>(this, EffectClass);
        UE_LOG(LogNomadAffliction, Warning, TEXT("Status effect pool exhausted, creating new instance"));
    }
    
    return Effect;
}
```

### Replication Optimization

```cpp
// Optimized replication structure
USTRUCT()
struct FNomadStatusEffectNetData
{
    GENERATED_BODY()
    
    // Compact representation for network transfer
    UPROPERTY()
    uint32 EffectID; // Compressed tag representation
    
    UPROPERTY()
    uint16 Duration; // Quantized duration (1 second precision)
    
    UPROPERTY()
    uint8 StackCount;
    
    UPROPERTY()
    uint8 Flags; // Packed boolean flags
    
    // Delta compression support
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);
};

// Custom replication component
UCLASS()
class NOMADDEV_API UNomadStatusEffectNetComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNomadStatusEffectNetComponent();
    
    // Efficient replication with delta compression
    UPROPERTY(Replicated)
    TArray<FNomadStatusEffectNetData> ReplicatedEffects;
    
    // Replicate only changes
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // Custom replication logic
    virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

private:
    // Track changes for delta replication
    TArray<FNomadStatusEffectNetData> PreviousEffects;
    
    void CompressEffectData(const TArray<FNomadStatusEffect>& Effects, TArray<FNomadStatusEffectNetData>& OutNetData);
    void DecompressEffectData(const TArray<FNomadStatusEffectNetData>& NetData, TArray<FNomadStatusEffect>& OutEffects);
};
```

---

## Debugging and Testing Tools

### Comprehensive Debug System

```cpp
// Debug visualization component
UCLASS(BlueprintType)
class NOMADDEV_API UNomadStatusEffectDebugComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNomadStatusEffectDebugComponent();
    
    // Debug display options
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
    bool bShowDebugInfo = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
    bool bShowTickTiming = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
    bool bShowNetworkReplication = false;
    
    // Debug rendering
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void DrawDebugStatusEffects();
    void DrawDebugTickTiming();
    void DrawDebugNetworkInfo();
    
    // Performance tracking
    TArray<float> TickTimes;
    float AverageTickTime = 0.0f;
    int32 EffectCount = 0;
};

// Console commands for debugging
class NOMADDEV_API FNomadStatusEffectConsoleCommands
{
public:
    static void RegisterCommands();

private:
    // Effect management commands
    static void CmdApplyEffect(const TArray<FString>& Args);
    static void CmdRemoveEffect(const TArray<FString>& Args);
    static void CmdListEffects(const TArray<FString>& Args);
    static void CmdClearAllEffects(const TArray<FString>& Args);
    
    // Debug commands
    static void CmdToggleDebugDisplay(const TArray<FString>& Args);
    static void CmdShowEffectDetails(const TArray<FString>& Args);
    static void CmdTestEffectConfig(const TArray<FString>& Args);
    
    // Performance commands
    static void CmdProfileEffects(const TArray<FString>& Args);
    static void CmdOptimizeEffects(const TArray<FString>& Args);
    
    // Helper functions
    static ACharacter* GetTargetCharacter();
    static void LogEffectInfo(const FNomadStatusEffect& Effect);
};

// Register console commands
void FNomadStatusEffectConsoleCommands::RegisterCommands()
{
    IConsoleManager& ConsoleManager = IConsoleManager::Get();
    
    // Effect management
    ConsoleManager.RegisterConsoleCommand(
        TEXT("se.apply"),
        TEXT("Apply status effect by tag. Usage: se.apply StatusEffect.Condition.Poison [duration]"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&CmdApplyEffect)
    );
    
    ConsoleManager.RegisterConsoleCommand(
        TEXT("se.remove"),
        TEXT("Remove status effect by tag. Usage: se.remove StatusEffect.Condition.Poison"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&CmdRemoveEffect)
    );
    
    ConsoleManager.RegisterConsoleCommand(
        TEXT("se.list"),
        TEXT("List all active status effects"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&CmdListEffects)
    );
    
    // Debug commands
    ConsoleManager.RegisterConsoleCommand(
        TEXT("se.debug"),
        TEXT("Toggle debug display for status effects"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&CmdToggleDebugDisplay)
    );
    
    ConsoleManager.RegisterConsoleCommand(
        TEXT("se.profile"),
        TEXT("Profile status effect performance"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&CmdProfileEffects)
    );
}
```

### Automated Testing Framework

```cpp
// Automated test suite for status effects
UCLASS()
class NOMADDEV_API UNomadStatusEffectTestSuite : public UObject
{
    GENERATED_BODY()

public:
    // Run all tests
    UFUNCTION(BlueprintCallable, Category="Testing")
    bool RunAllTests();
    
    // Individual test categories
    UFUNCTION(BlueprintCallable, Category="Testing")
    bool TestEffectApplication();
    
    UFUNCTION(BlueprintCallable, Category="Testing")
    bool TestEffectStacking();
    
    UFUNCTION(BlueprintCallable, Category="Testing")
    bool TestEffectRemoval();
    
    UFUNCTION(BlueprintCallable, Category="Testing")
    bool TestConfigValidation();
    
    UFUNCTION(BlueprintCallable, Category="Testing")
    bool TestNetworkReplication();

private:
    // Test infrastructure
    ACharacter* CreateTestCharacter();
    void CleanupTestCharacter(ACharacter* Character);
    bool VerifyEffectState(ACharacter* Character, FGameplayTag EffectTag, bool bShouldExist);
    
    // Test cases
    bool TestBasicApplication();
    bool TestDurationExpiration();
    bool TestStackingBehavior();
    bool TestConflictResolution();
    bool TestSaveLoadPersistence();
    
    // Test results
    TArray<FString> TestResults;
    int32 PassedTests = 0;
    int32 FailedTests = 0;
    
    void LogTestResult(const FString& TestName, bool bPassed, const FString& Details = TEXT(""));
};

// Example test implementation
bool UNomadStatusEffectTestSuite::TestEffectApplication()
{
    ACharacter* TestCharacter = CreateTestCharacter();
    if (!TestCharacter)
    {
        LogTestResult(TEXT("Effect Application"), false, TEXT("Failed to create test character"));
        return false;
    }
    
    UNomadStatusEffectManagerComponent* SEManager = TestCharacter->GetComponentByClass<UNomadStatusEffectManagerComponent>();
    if (!SEManager)
    {
        LogTestResult(TEXT("Effect Application"), false, TEXT("No status effect manager on test character"));
        CleanupTestCharacter(TestCharacter);
        return false;
    }
    
    // Test basic application
    FGameplayTag TestEffectTag = FGameplayTag::RequestGameplayTag("StatusEffect.Test.BasicEffect");
    bool bApplied = SEManager->ApplyStatusEffect(UNomadTimedStatusEffect::StaticClass(), TestEffectTag);
    
    if (!bApplied)
    {
        LogTestResult(TEXT("Effect Application"), false, TEXT("Failed to apply test effect"));
        CleanupTestCharacter(TestCharacter);
        return false;
    }
    
    // Verify effect is active
    bool bHasEffect = SEManager->HasActiveEffect(TestEffectTag);
    if (!bHasEffect)
    {
        LogTestResult(TEXT("Effect Application"), false, TEXT("Effect not found after application"));
        CleanupTestCharacter(TestCharacter);
        return false;
    }
    
    CleanupTestCharacter(TestCharacter);
    LogTestResult(TEXT("Effect Application"), true);
    return true;
}
```

---

This Developer Implementation Guide provides the technical foundation for extending and maintaining the Nomad Survival System. For configuration workflows, see the [Designer Guide](NomadSurvivalSystem_Designer_Guide.md). For quick reference, check the [Quick Reference Guide](NomadSurvivalSystem_QuickReference.md).