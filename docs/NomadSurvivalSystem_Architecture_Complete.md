# Nomad Survival System - Complete Architecture Guide

**Version:** 1.0  
**Last Updated:** 2025-01-27  
**Audience:** Designers, Developers, Technical Artists  

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Architecture Diagrams](#architecture-diagrams)
3. [Core Components](#core-components)
4. [Data Flow](#data-flow)
5. [Status Effect Hierarchy](#status-effect-hierarchy)
6. [Configuration System](#configuration-system)
7. [Integration Points](#integration-points)
8. [Performance Considerations](#performance-considerations)
9. [Multiplayer Architecture](#multiplayer-architecture)
10. [Real-World Examples](#real-world-examples)

---

## System Overview

The Nomad Survival System is a comprehensive status effect framework built on top of the Ascent Combat Framework (ACF). It provides a data-driven, flexible system for managing survival mechanics including hunger, thirst, temperature effects, and various status conditions.

### Key Features

- **Data-Driven Configuration**: All values configurable through Unreal Engine DataAssets
- **Hybrid Application System**: Supports both direct stat modification and UE damage events
- **Multiplayer Ready**: Full replication support through ACF
- **Modular Design**: Easy to extend with new effect types
- **Designer-Friendly**: Visual configuration with validation
- **Performance Optimized**: Efficient tick management and memory usage

### Core Philosophy

1. **No Hardcoded Values**: Everything configurable through assets
2. **Separation of Concerns**: Logic, data, and presentation are decoupled
3. **Extensibility**: Easy to add new survival mechanics
4. **Type Safety**: Strong typing and validation throughout
5. **Performance First**: Optimized for gameplay at scale

---

## Architecture Diagrams

### High-Level System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    NOMAD SURVIVAL SYSTEM                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐    ┌──────────────────┐    ┌────────────┐ │
│  │ Player Character│    │ Survival Needs   │    │ Status     │ │
│  │                 │◄───┤ Component        │◄───┤ Effects    │ │
│  │ - Health        │    │                  │    │ Manager    │ │
│  │ - Hunger        │    │ - Hunger Logic   │    │            │ │
│  │ - Thirst        │    │ - Thirst Logic   │    │ - Apply    │ │
│  │ - Temperature   │    │ - Temperature    │    │ - Remove   │ │
│  └─────────────────┘    │ - Status Effects │    │ - Stack    │ │
│                         └──────────────────┘    └────────────┘ │
│                                                                 │
│  ┌─────────────────┐    ┌──────────────────┐    ┌────────────┐ │
│  │ Config Assets   │    │ Base Status      │    │ Specialized│ │
│  │                 │    │ Effect           │    │ Effects    │ │
│  │ - Effect Configs│    │                  │    │            │ │
│  │ - Hazard Configs│    │ - Lifecycle      │    │ - Survival │ │
│  │ - UI Configs    │    │ - Application    │    │ - Timed    │ │
│  │                 │    │ - Replication    │    │ - Infinite │ │
│  └─────────────────┘    └──────────────────┘    │ - Instant  │ │
│                                                  └────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Status Effect Class Hierarchy

```
UNomadBaseStatusEffect (Abstract)
├── Application Logic
├── Configuration Loading
├── Lifecycle Management
└── Replication Support

├─ UNomadInstantStatusEffect
│  └── Immediate application (healing potions, instant damage)

├─ UNomadTimedStatusEffect  
│  └── Duration-based effects (buffs, debuffs, temporary conditions)

├─ UNomadInfiniteStatusEffect
│  └── Permanent until removed (equipment bonuses, persistent states)

└─ UNomadSurvivalStatusEffect (extends Infinite)
   └── Survival mechanics (starvation, dehydration, temperature)
```

### Data Flow Architecture

```
Configuration Assets → Status Effects → Game State → UI Updates
        │                    │              │           │
        │                    │              │           └─ Notifications
        │                    │              │           └─ Health Bars  
        │                    │              │           └─ Status Icons
        │                    │              │
        │                    │              └─ Attribute Changes
        │                    │              └─ Movement Modifiers
        │                    │              └─ Input Blocking
        │                    │
        │                    └─ Damage Events
        │                    └─ Stat Modifications
        │                    └─ Audio/Visual Effects
        │
        └─ Validation
        └─ Default Values
        └─ Designer Guidelines
```

---

## Core Components

### 1. UNomadSurvivalNeedsComponent

**Purpose**: Manages the core survival mechanics (hunger, thirst, temperature)  
**Location**: `NomadSource/NomadDev/Public/Core/Component/NomadSurvivalNeedsComponent.h`

**Key Responsibilities**:
- Tracks survival stats (hunger, thirst, body temperature)
- Applies decay over time based on configured rates
- Triggers status effects when thresholds are crossed
- Handles stat recovery and normalization
- Manages exposure effects (heat/cold damage)

**Important Methods**:
```cpp
// Stat management
void ModifyHunger(float Amount);
void ModifyThirst(float Amount);
void ModifyBodyTemperature(float Amount);

// Status effect integration
void ApplySurvivalStatusEffect(ESurvivalSeverity Severity, FGameplayTag EffectTag);
void RemoveSurvivalStatusEffect(FGameplayTag EffectTag);

// Configuration
void LoadSurvivalConfig();
```

### 2. UNomadStatusEffectManagerComponent

**Purpose**: Central manager for all status effects on a character  
**Location**: `NomadSource/NomadDev/Public/Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h`

**Key Responsibilities**:
- Applies and removes status effects
- Manages effect stacking and duration
- Handles replication in multiplayer
- Provides query methods for active effects
- Manages blocking tags for input restrictions

**Important Methods**:
```cpp
// Effect management
bool ApplyStatusEffect(TSubclassOf<UNomadBaseStatusEffect> EffectClass);
bool RemoveStatusEffect(FGameplayTag EffectTag);
bool HasActiveEffect(FGameplayTag EffectTag);

// Blocking system
bool IsActionBlocked(FGameplayTag ActionTag);
void ApplyBlockingTag(FGameplayTag BlockingTag);
void RemoveBlockingTag(FGameplayTag BlockingTag);
```

### 3. Configuration Asset System

**Base Class**: `UNomadStatusEffectConfigBase`  
**Purpose**: Data-driven configuration for all status effects

**Configuration Types**:

#### UNomadInfiniteEffectConfig
```cpp
// Persistent effects (equipment bonuses, survival conditions)
- PersistentAttributeModifier: Map of stat changes
- BlockingTags: Actions to disable
- Category: Positive/Negative/Neutral
- VisualEffects: Particles, screen effects
- AudioCues: Sound effects for apply/remove
```

#### UNomadTimedEffectConfig  
```cpp
// Temporary effects (buffs, debuffs, potions)
- Duration: Effect length in seconds
- TickInterval: How often effect processes
- StatModifiersPerTick: Damage/healing per tick
- StackingRules: How multiple applications behave
```

#### UNomadSurvivalHazardConfig
```cpp
// Survival-specific configurations
struct FNomadHazardConfigRow:
- HazardTag: Unique identifier
- EffectClass: Which effect class to use
- DoTPercent: Damage as percentage of health per second
- StatType: Which survival stat this affects
- VisualCue: VFX/audio descriptions
```

---

## Data Flow

### 1. Status Effect Application Flow

```
1. Trigger Event (stat threshold, external call, etc.)
   │
2. UNomadSurvivalNeedsComponent.ApplySurvivalStatusEffect()
   │
3. UNomadStatusEffectManagerComponent.ApplyStatusEffect()
   │
4. Create UNomadSurvivalStatusEffect instance
   │
5. Load configuration from DataAsset
   │
6. UNomadBaseStatusEffect.ApplyBaseConfiguration()
   │
7. Apply attribute modifiers (movement speed, health, etc.)
   │
8. Apply blocking tags (prevent jumping, sprinting)
   │
9. Trigger visual/audio effects
   │
10. Start effect lifecycle (infinite, timed, etc.)
    │
11. Replicate to clients (if multiplayer)
    │
12. Update UI notifications
```

### 2. Damage Over Time (DoT) Flow

```
1. UNomadSurvivalStatusEffect.HandleInfiniteTick()
   │
2. Load hazard config (DoTPercent, damage type)
   │
3. Calculate damage amount:
   │  HealthDamage = MaxHealth * DoTPercent * DeltaTime
   │
4. Choose application method:
   │
   ├─ StatModification Mode:
   │  └─ Directly modify health attribute
   │
   ├─ DamageEvent Mode:
   │  └─ Create UDamageEvent and apply through UE pipeline
   │
   └─ Both Mode:
      └─ Apply both stat and damage event
      
5. Sync movement speed if health affects mobility
   │
6. Check for death condition
   │
7. Update UI health bars
```

### 3. Configuration Loading Flow

```
1. Status effect created/initialized
   │
2. UNomadBaseStatusEffect.GetEffectConfig()
   │
3. EffectConfig.LoadSynchronous() - Load DataAsset
   │
4. Validate configuration:
   │  - Required fields present
   │  - Values within acceptable ranges
   │  - References are valid
   │
5. UNomadBaseStatusEffect.LoadConfigurationValues()
   │
6. Apply configuration:
   │  - Set effect name, description, icon
   │  - Configure damage/healing values  
   │  - Set blocking tags
   │  - Configure audio/visual effects
   │
7. Effect ready for application
```

---

## Status Effect Hierarchy

### Base Class: UNomadBaseStatusEffect

**Core Features**:
- Configuration asset loading and validation
- Lifecycle state management (Active, Ending, Removed)
- Audio/visual effect integration
- Blocking tag system for input restrictions
- Replication support through ACF
- Category system for UI organization

**Key Virtual Methods**:
```cpp
// Override in derived classes
virtual void OnStatusEffectStarts_Implementation();
virtual void OnStatusEffectEnds_Implementation();
virtual bool CanBeApplied_Implementation();
virtual ENomadStatusCategory GetStatusCategory_Implementation();
```

### Specialized Classes

#### UNomadInstantStatusEffect
**Use Cases**: Healing potions, instant damage, one-time buffs
**Behavior**: Applies effect immediately and removes itself
**Example**: Health potion that instantly restores 50 HP

#### UNomadTimedStatusEffect  
**Use Cases**: Temporary buffs/debuffs, potion effects, environmental hazards
**Behavior**: Runs for a specified duration, can tick periodically
**Example**: Speed boost that lasts 30 seconds with +25% movement speed

#### UNomadInfiniteStatusEffect
**Use Cases**: Equipment bonuses, persistent conditions
**Behavior**: Remains active until explicitly removed
**Example**: Heavy armor that reduces movement speed by 15%

#### UNomadSurvivalStatusEffect
**Use Cases**: Survival mechanics (hunger, thirst, temperature)
**Behavior**: Inherits from Infinite, adds severity levels and survival-specific logic
**Example**: Starvation effect that deals 0.5% health damage per second

---

## Configuration System

### DataAsset-Driven Design

All status effects are configured through Unreal Engine DataAssets, providing:
- **Visual Editing**: Configure effects in the UE editor with validation
- **Version Control**: Assets can be tracked and merged like code
- **Modding Support**: Easy for content creators to modify
- **Runtime Safety**: Validation prevents invalid configurations

### Configuration Validation

Each config asset includes validation to prevent common mistakes:

```cpp
bool UNomadStatusEffectConfigBase::IsConfigValid() const
{
    // Check required fields
    if (EffectTag.IsValid() == false) return false;
    if (EffectName.IsEmpty()) return false;
    
    // Validate damage values
    if (ApplicationMode != EStatusEffectApplicationMode::StatModification)
    {
        if (!DamageTypeClass) return false;
        if (DamageValues.IsEmpty()) return false;
    }
    
    // Validate attribute modifiers
    for (const auto& Modifier : PersistentAttributeModifier)
    {
        if (Modifier.Value <= 0.0f) return false; // Must be positive multiplier
    }
    
    return true;
}
```

### Configuration Examples

#### Starvation Effect Configuration
```cpp
// Asset: NomadSurvivalHazardConfig
FNomadHazardConfigRow StarvationConfig:
{
    Name: "Starvation"
    HazardTag: "StatusEffect.Survival.Starvation"
    EffectClass: UNomadSurvivalStatusEffect
    DoTPercent: 0.005f  // 0.5% health damage per second
    StatType: "HUNGER"
    UIType: "BAR"
    Gameplay: "Deals health damage when hunger reaches 0"
    VisualCue: "Screen desaturation, stomach rumble sounds"
    DesignerNotes: "Should feel urgent but not instantly lethal"
}
```

#### Movement Speed Debuff Configuration
```cpp
// Asset: UNomadInfiniteEffectConfig  
{
    EffectName: "Exhaustion"
    Description: "Heavy fatigue reducing movement capabilities"
    EffectTag: "StatusEffect.Condition.Exhaustion"
    Category: Negative
    
    PersistentAttributeModifier:
    {
        "RPG.Attributes.MovementSpeed": 0.7f  // 30% speed reduction
    }
    
    BlockingTags:
    [
        "Status.Block.Sprint",  // Cannot sprint when exhausted
        "Status.Block.Jump"     // Cannot jump when exhausted  
    ]
    
    ApplicationMode: StatModification
}
```

---

## Integration Points

### 1. Attribute System Integration

The survival system integrates with the Advanced RPG System (ARS) for stat management:

```cpp
// Get player's statistics component
UARSStatisticsComponent* StatsComponent = Character->GetComponentByClass<UARSStatisticsComponent>();

// Modify attributes through status effects
StatusEffect->ApplyAttributeModifier("RPG.Statistics.Health", -10.0f);
StatusEffect->ApplyAttributeModifier("RPG.Attributes.MovementSpeed", 0.8f);

// Automatic synchronization
UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromDefaultAttribute(Character);
```

### 2. Movement System Integration

Status effects can modify movement through:

**Direct Attribute Modification**:
```cpp
// Configure in DataAsset
PersistentAttributeModifier.Add("RPG.Attributes.MovementSpeed", 0.5f); // 50% speed
```

**Input Blocking**:
```cpp
// Configure blocking tags
BlockingTags.AddTag("Status.Block.Sprint");
BlockingTags.AddTag("Status.Block.Jump");

// Check in movement code
if (StatusEffectManager->IsActionBlocked(FGameplayTag::RequestGameplayTag("Status.Block.Sprint")))
{
    // Disable sprinting
}
```

### 3. UI System Integration

Status effects provide rich data for UI systems:

```cpp
// Get active effects for UI display
TArray<FNomadStatusEffect> ActiveEffects = StatusEffectManager->GetActiveEffects();

// Display effect information
for (const FNomadStatusEffect& Effect : ActiveEffects)
{
    // Show icon, name, duration, category
    UTexture2D* Icon = Effect.GetIcon();
    FText Name = Effect.GetEffectName();
    ENomadStatusCategory Category = Effect.GetCategory();
    
    // Color-code by category
    FLinearColor UIColor = (Category == ENomadStatusCategory::Positive) ? 
        FLinearColor::Green : FLinearColor::Red;
}
```

### 4. Audio/Visual Effects Integration

Status effects can trigger audio and visual feedback:

```cpp
// Configure in DataAsset
VisualEffects:
{
    StartEffect: "VFX_StatusEffect_Poison_Start"
    LoopEffect: "VFX_StatusEffect_Poison_Loop"  
    EndEffect: "VFX_StatusEffect_Poison_End"
}

AudioCues:
{
    ApplySound: "SFX_Poison_Apply"
    TickSound: "SFX_Poison_Tick"
    RemoveSound: "SFX_Poison_Cure"
}

// Automatic playback during effect lifecycle
virtual void OnStatusEffectStarts_Implementation() override
{
    Super::OnStatusEffectStarts_Implementation();
    PlayConfiguredVisualEffect(EVisualEffectType::StartEffect);
    PlayConfiguredAudioCue(EAudioCueType::ApplySound);
}
```

---

## Performance Considerations

### 1. Tick Management

The system uses intelligent tick management to minimize performance impact:

**Tick Grouping**: Related effects are ticked together to reduce overhead
**Dynamic Intervals**: Tick rates adjust based on effect urgency
**Batch Processing**: Multiple effects processed in single pass

```cpp
// Configurable tick intervals
UNomadTimedStatusEffect:
{
    TickInterval: 1.0f    // Most effects tick once per second
    FastTickInterval: 0.1f // Critical effects can tick faster
    SlowTickInterval: 5.0f // Background effects tick slower
}
```

### 2. Memory Management

**Object Pooling**: Status effects are pooled and reused
**Smart Loading**: Configuration assets loaded on-demand
**Garbage Collection**: Proper cleanup prevents memory leaks

```cpp
// Status effect pooling
class UNomadStatusEffectPool
{
    TArray<UNomadBaseStatusEffect*> AvailableEffects;
    TArray<UNomadBaseStatusEffect*> ActiveEffects;
    
    UNomadBaseStatusEffect* GetPooledEffect(TSubclassOf<UNomadBaseStatusEffect> EffectClass);
    void ReturnEffectToPool(UNomadBaseStatusEffect* Effect);
};
```

### 3. Replication Optimization

**Delta Compression**: Only changed effect data is replicated
**Relevancy Filtering**: Effects only replicated to relevant clients
**Batch Updates**: Multiple effect changes combined into single RPC

```cpp
// Optimized replication
UPROPERTY(ReplicatedUsing = OnRep_StatusEffects)
TArray<FStatusEffect> ReplicatedStatusEffects;

// Only replicate when effects change
bool bStatusEffectsChanged = false;
void MarkStatusEffectsDirty() { bStatusEffectsChanged = true; }
```

### 4. Configuration Caching

**Asset Caching**: Frequently used configs cached in memory
**Validation Caching**: Validation results cached to avoid repeated checks
**Lookup Optimization**: Tag-based lookups use hash maps for O(1) access

---

## Multiplayer Architecture

### 1. Authority Model

The system follows Unreal Engine's client-server authority model:

**Server Authority**: 
- All status effect applications/removals happen on server
- Server validates all effect requests
- Server manages authoritative game state

**Client Prediction**:
- Clients can predict effect outcomes for responsiveness  
- Server corrections override client predictions
- UI updates immediately while awaiting server confirmation

### 2. Replication Strategy

```cpp
// Status effects replicate through ACF's proven system
class UNomadStatusEffectManagerComponent : public UACFStatusEffectManagerComponent
{
    // Inherits reliable replication from ACF
    // Automatic delta compression
    // Built-in relevancy filtering
};

// Custom replication for survival-specific data
UPROPERTY(Replicated)
float ReplicatedHungerLevel;

UPROPERTY(Replicated)  
float ReplicatedThirstLevel;

// Efficient replication only when values change
void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override
{
    DOREPLIFETIME_CONDITION(UNomadSurvivalNeedsComponent, ReplicatedHungerLevel, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UNomadSurvivalNeedsComponent, ReplicatedThirstLevel, COND_OwnerOnly);
}
```

### 3. Network Security

**Server Validation**:
- All effect applications validated on server
- Anti-cheat measures prevent invalid effect modifications
- Rate limiting prevents effect spam

**Data Integrity**:
- Configuration assets verified on both client and server
- Checksums ensure asset consistency
- Invalid configurations rejected with logging

---

## Real-World Examples

### Example 1: Starvation System

**Scenario**: Player's hunger drops to 0, triggering starvation

**Configuration**:
```cpp
// NomadSurvivalHazardConfig DataAsset
StarvationRow:
{
    Name: "Starvation"
    HazardTag: "StatusEffect.Survival.Starvation"  
    EffectClass: UNomadSurvivalStatusEffect
    DoTPercent: 0.005f  // 0.5% max health per second
    StatType: "HUNGER"
    Gameplay: "Reduces health until hunger is restored"
    VisualCue: "Screen desaturation, audio heartbeat"
}
```

**Execution Flow**:
1. `UNomadSurvivalNeedsComponent` detects hunger ≤ 0
2. Calls `ApplySurvivalStatusEffect(ESurvivalSeverity::Severe, StarvationTag)`
3. `UNomadSurvivalStatusEffect` created and configured
4. Effect begins infinite tick loop with 0.5% health damage per second
5. Movement speed reduced by 25% (configured in effect asset)
6. Jump and sprint actions blocked (configured blocking tags)
7. UI shows starvation icon and health decay
8. Audio plays stomach rumbling and heartbeat sounds
9. Screen gradually desaturates to convey weakness
10. Effect continues until hunger is restored above threshold

### Example 2: Temperature Management

**Scenario**: Player enters extreme heat, triggering heatstroke

**Configuration**:
```cpp
// Severe Heat Effect
HeatstrokeRow:
{
    Name: "Heatstroke"
    HazardTag: "StatusEffect.Survival.Heatstroke"
    EffectClass: UNomadSurvivalStatusEffect
    DoTPercent: 0.0f    // No direct health damage
    StatType: "HOT"
    Gameplay: "Increases thirst consumption by 4x"
    VisualCue: "Heat shimmer, sweat effects, red screen tint"
}

// Corresponding UNomadInfiniteEffectConfig
HeatstrokeEffectConfig:
{
    EffectName: "Heatstroke"
    Category: Negative
    PersistentAttributeModifier:
    {
        "RPG.Attributes.ThirstDecayRate": 4.0f  // 4x thirst consumption
    }
    BlockingTags: ["Status.Block.Sprint"]  // Can't sprint in extreme heat
}
```

**Execution Flow**:
1. Player's body temperature rises above extreme heat threshold
2. `UNomadSurvivalNeedsComponent` applies heatstroke effect
3. Thirst decay rate multiplied by 4x through attribute modifier
4. Sprint action blocked to prevent overheating
5. Visual effects: Heat shimmer particles, red screen tint
6. Audio: Heavy breathing, heat ambience
7. UI shows temperature warning and increased thirst consumption
8. Effect persists until player moves to cooler environment
9. Gradual cooldown removes effect with transition animations

### Example 3: Equipment-Based Status Effects

**Scenario**: Player equips heavy armor that reduces mobility

**Configuration**:
```cpp
// Heavy Armor Effect Config (UNomadInfiniteEffectConfig)
HeavyArmorEffectConfig:
{
    EffectName: "Heavy Armor"
    Description: "Protective but reduces mobility"
    EffectTag: "StatusEffect.Equipment.HeavyArmor"
    Category: Neutral  // Not positive or negative, just different
    
    PersistentAttributeModifier:
    {
        "RPG.Attributes.MovementSpeed": 0.75f      // 25% speed reduction
        "RPG.Attributes.StaminaRegen": 0.8f        // 20% slower stamina regen
        "RPG.Statistics.ArmorRating": 1.5f         // 50% more armor
    }
    
    BlockingTags: []  // No actions blocked, just modified
    ApplicationMode: StatModification
}
```

**Execution Flow**:
1. Player equips heavy armor in inventory system
2. Equipment system calls `StatusEffectManager->ApplyStatusEffect(HeavyArmorEffect)`
3. `UNomadInfiniteStatusEffect` applies configured attribute modifiers
4. Movement speed reduced to 75% through character movement component
5. Stamina regeneration slowed for realistic weight simulation
6. Armor rating increased for protection benefit
7. UI shows equipment effect in status bar with neutral (yellow) color
8. Effect persists while armor is equipped
9. Removing armor automatically removes the status effect
10. All attribute modifications cleanly restored to baseline

### Example 4: Bleeding (Stackable DoT)

**Scenario**: Player takes damage and starts bleeding, multiple wounds stack

**Configuration**:
```cpp
// Bleeding Effect Config (UNomadTimedEffectConfig)
BleedingEffectConfig:
{
    EffectName: "Bleeding"
    Description: "Losing blood from wounds"  
    EffectTag: "StatusEffect.Condition.Bleeding"
    Category: Negative
    Duration: 30.0f     // Bleeds for 30 seconds
    TickInterval: 1.0f  // Damage every second
    CanStack: true      // Multiple wounds can stack
    MaxStacks: 5        // Maximum 5 bleeding wounds
    
    StatModifiersPerTick:
    {
        "RPG.Statistics.Health": -1.0f  // 1 HP lost per second per stack
    }
    
    ApplicationMode: DamageEvent  // Use UE damage system for resistances
    DamageTypeClass: UBleedingDamageType
}
```

**Execution Flow**:
1. Player takes slashing damage from enemy attack
2. Damage system applies bleeding effect as secondary consequence
3. First bleeding stack starts: 1 HP/second for 30 seconds
4. Player takes another wound before first heals
5. Second bleeding stack applied: now 2 HP/second total
6. Each stack shows separately in UI with stack counter
7. Visual effects: Blood drip particles, periodic red screen flash
8. Audio: Heartbeat intensifies with more stacks
9. Player can use bandages to remove bleeding stacks
10. Each stack expires independently after 30 seconds
11. UI updates dynamically as stacks are added/removed

---

This architecture guide provides the foundation for understanding how the Nomad Survival System works at a technical level. For practical implementation details, see the [Developer Guide](NomadSurvivalSystem_Developer_Guide.md). For configuration workflows, see the [Designer Guide](NomadSurvivalSystem_Designer_Guide.md).