# Nomad Survival System - Designer Configuration Guide

**Version:** 1.0  
**Last Updated:** 2025-01-27  
**Audience:** Game Designers, Content Creators, Level Designers  

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Configuration Workflow](#configuration-workflow)
3. [Status Effect Types](#status-effect-types)
4. [Step-by-Step Tutorials](#step-by-step-tutorials)
5. [Configuration Examples](#configuration-examples)
6. [Asset Requirements](#asset-requirements)
7. [UI Integration](#ui-integration)
8. [Testing and Validation](#testing-and-validation)
9. [Common Scenarios](#common-scenarios)
10. [Troubleshooting](#troubleshooting)
11. [Best Practices](#best-practices)

---

## Getting Started

### What You Need to Know

The Nomad Survival System lets you create and configure status effects without programming. Everything is done through **DataAssets** in the Unreal Engine editor with visual configuration and built-in validation.

### Core Concepts

- **Status Effects**: Conditions that affect the player (hunger, buffs, debuffs)
- **DataAssets**: Configuration files that define how effects behave
- **Effect Types**: Different behaviors (instant, timed, infinite, survival)
- **Severity Levels**: How intense an effect is (mild, severe, extreme)
- **Blocking Tags**: Actions the effect prevents (jumping, sprinting)

### Before You Start

1. **Understand the Game's Vision**: Know what survival experience you want to create
2. **Check Existing Assets**: Review current effects to maintain consistency
3. **Plan Your Effect**: Define behavior, duration, visual feedback before configuring
4. **Test Early and Often**: Use in-game testing to validate your designs

---

## Configuration Workflow

### Phase 1: Design Planning

**1. Define the Effect Purpose**
- What gameplay need does this effect serve?
- How should it feel to the player?
- What visual/audio feedback is appropriate?

**2. Determine Effect Type**
- **Instant**: Immediate one-time effect (healing potion)
- **Timed**: Temporary effect with duration (speed boost)
- **Infinite**: Permanent until removed (equipment bonus)
- **Survival**: Survival mechanics (starvation, dehydration)

**3. Plan Integration Points**
- Which stats/attributes does it affect?
- What actions should it block or modify?
- How does it interact with other effects?

### Phase 2: Asset Creation

**1. Create Configuration DataAsset**
```
Right-click in Content Browser
→ Miscellaneous 
→ Data Asset
→ Choose appropriate config type:
   - NomadInstantEffectConfig
   - NomadTimedEffectConfig  
   - NomadInfiniteEffectConfig
   - NomadSurvivalHazardEffectConfig
```

**2. Configure Basic Information**
```
Effect Name: [User-friendly name for UI]
Description: [Tooltip text for players]
Effect Tag: [Unique identifier - use hierarchical naming]
Category: [Positive/Negative/Neutral for UI color coding]
Icon: [UI icon texture asset]
```

**3. Set Up Gameplay Effects**
```
Application Mode: [How effect applies damage/healing]
- StatModification: Direct stat changes (fast, simple)
- DamageEvent: Uses UE damage system (supports resistances)
- Both: Applies both methods (use carefully)

Persistent Attribute Modifier: [Map of stat changes]
- Key: Attribute tag (e.g., "RPG.Attributes.MovementSpeed")
- Value: Multiplier (0.8 = 20% reduction, 1.2 = 20% increase)

Blocking Tags: [Actions to prevent]
- "Status.Block.Jump"
- "Status.Block.Sprint"  
- "Status.Block.Interact"
```

### Phase 3: Testing and Iteration

**1. In-Editor Testing**
- Use Blueprint testing functions
- Validate configuration with built-in checks
- Test with placeholder assets initially

**2. In-Game Testing**
- Apply effects through console commands
- Test with multiple effects active
- Verify visual/audio feedback works
- Check UI integration

**3. Balance Testing**
- Test effect in actual gameplay scenarios
- Get feedback from playtesters
- Iterate on values based on player experience

---

## Status Effect Types

### Instant Effects
**When to Use**: One-time application (healing items, instant damage, buffs that apply once)

**Key Configuration**:
```
Duration: 0 (applied immediately)
Tick Interval: N/A (no ticking)
Application: Happens once on apply
```

**Example Use Cases**:
- Healing potions that restore health immediately
- Instant damage from traps or explosions
- Buffs that increase max health permanently

### Timed Effects
**When to Use**: Temporary conditions with fixed duration (buff/debuff spells, potion effects)

**Key Configuration**:
```
Duration: [Time in seconds]
Tick Interval: [How often it processes] 
Can Stack: [Whether multiple applications stack]
Max Stacks: [Limit on stacking]
Stat Modifiers Per Tick: [Damage/healing per tick]
```

**Example Use Cases**:
- Speed boost potions (30 seconds, +25% speed)
- Damage over time effects (poison, burning)
- Temporary attribute bonuses from spells

### Infinite Effects  
**When to Use**: Permanent conditions until explicitly removed (equipment bonuses, persistent states)

**Key Configuration**:
```
Duration: Infinite (until removed)
Persistent Attribute Modifier: [Permanent stat changes]
Blocking Tags: [Actions to prevent]
```

**Example Use Cases**:
- Equipment bonuses (armor reduces speed)
- Character states (diseases, blessings)
- Environmental effects in specific areas

### Survival Effects
**When to Use**: Core survival mechanics (hunger, thirst, temperature effects)

**Key Configuration**:
```
Severity Level: [Mild/Moderate/Severe/Extreme]
DoT Percent: [Percentage of health lost per second]
Stat Type: [Which survival stat this affects]
Auto-removal: [When survival stat improves]
```

**Example Use Cases**:
- Starvation when hunger reaches 0
- Dehydration when thirst is depleted
- Heatstroke in extreme temperatures

---

## Step-by-Step Tutorials

### Tutorial 1: Creating a Speed Boost Potion

**Goal**: Create a temporary speed boost effect from drinking a potion

**Step 1: Create the DataAsset**
1. Right-click in Content Browser → Data Asset
2. Select `NomadTimedEffectConfig`
3. Name it `SpeedBoostPotionEffect`

**Step 2: Configure Basic Info**
```
Effect Name: "Speed Boost"
Description: "Increased movement speed from magical enhancement"
Effect Tag: "StatusEffect.Potion.SpeedBoost"
Category: Positive
Icon: [Select speed boost icon texture]
```

**Step 3: Configure Duration and Timing**
```
Duration: 30.0 (30 seconds)
Tick Interval: 1.0 (check every second, though not needed for this effect)
Can Stack: false (drinking multiple potions doesn't stack)
Max Stacks: 1
```

**Step 4: Configure Movement Speed**
```
Application Mode: StatModification

Persistent Attribute Modifier:
- Add entry: Key = "RPG.Attributes.MovementSpeed", Value = 1.25
  (This gives 25% speed increase: 100% base + 25% bonus = 125% total)
```

**Step 5: Configure Visual/Audio (Optional)**
```
Visual Effects:
- Start Effect: "VFX_SpeedBoost_Apply" (particle effect when applied)
- Loop Effect: "VFX_SpeedBoost_Loop" (ongoing visual while active)
- End Effect: "VFX_SpeedBoost_End" (visual when effect ends)

Audio Cues:
- Apply Sound: "SFX_Potion_SpeedBoost"
- Remove Sound: "SFX_Buff_Expire"
```

**Step 6: Test the Effect**
```
In-game console commands:
se.apply StatusEffect.Potion.SpeedBoost
se.remove StatusEffect.Potion.SpeedBoost
se.list (to see active effects)
```

### Tutorial 2: Creating Bleeding Effect (Stackable DoT)

**Goal**: Create a bleeding effect that can stack from multiple wounds

**Step 1: Create the DataAsset**
1. Create `NomadTimedEffectConfig`
2. Name it `BleedingEffect`

**Step 2: Configure Basic Info**
```
Effect Name: "Bleeding"
Description: "Losing blood from wounds - seek medical attention"
Effect Tag: "StatusEffect.Condition.Bleeding"
Category: Negative
Icon: [Blood drop icon]
```

**Step 3: Configure Stacking Behavior**
```
Duration: 30.0 (30 seconds per wound)
Tick Interval: 1.0 (damage every second)
Can Stack: true (multiple wounds can bleed)
Max Stacks: 5 (maximum 5 bleeding wounds)
```

**Step 4: Configure Damage Over Time**
```
Application Mode: DamageEvent (uses UE damage system for resistances)
Damage Type Class: BleedingDamageType

Stat Modifiers Per Tick:
- Add entry: Key = "RPG.Statistics.Health", Value = -1.0
  (Loses 1 HP per second per stack)
```

**Step 5: Configure Feedback**
```
Visual Effects:
- Loop Effect: "VFX_Bleeding_Drip" (blood drip particles)

Audio Cues:
- Apply Sound: "SFX_Wound_Bleed"
- Tick Sound: "SFX_Heartbeat_Stressed" (plays each damage tick)
```

**Step 6: Test Stacking**
```
Console commands:
se.apply StatusEffect.Condition.Bleeding  (apply first stack)
se.apply StatusEffect.Condition.Bleeding  (apply second stack)
se.list  (should show 2 stacks, 2 HP/second total damage)
```

### Tutorial 3: Creating Heavy Armor Encumbrance

**Goal**: Create an equipment effect that reduces mobility but increases protection

**Step 1: Create the DataAsset**
1. Create `NomadInfiniteEffectConfig`
2. Name it `HeavyArmorEffect`

**Step 2: Configure Basic Info**
```
Effect Name: "Heavy Armor"
Description: "Protective plating that restricts movement"
Effect Tag: "StatusEffect.Equipment.HeavyArmor"
Category: Neutral (not purely positive or negative)
Icon: [Heavy armor icon]
```

**Step 3: Configure Stat Modifications**
```
Application Mode: StatModification

Persistent Attribute Modifier:
- "RPG.Attributes.MovementSpeed": 0.75 (25% speed reduction)
- "RPG.Attributes.StaminaRegen": 0.8 (20% slower stamina recovery)
- "RPG.Statistics.ArmorRating": 1.5 (50% more armor protection)
```

**Step 4: Configure Action Restrictions**
```
Blocking Tags:
- (Leave empty - heavy armor doesn't block actions, just modifies them)

Note: If you wanted to prevent certain actions:
- "Status.Block.Sprint" (can't sprint in heavy armor)
- "Status.Block.Dodge" (can't dodge roll)
```

**Step 5: Test Equipment Integration**
```
Console commands:
se.apply StatusEffect.Equipment.HeavyArmor   (apply when equipped)
se.remove StatusEffect.Equipment.HeavyArmor  (remove when unequipped)

Check movement speed in game - should be visibly slower
Check armor rating in character stats - should be higher
```

### Tutorial 4: Creating Starvation Survival Effect

**Goal**: Create the core starvation mechanic for the survival system

**Step 1: Create the DataAsset**
1. Create `NomadSurvivalHazardConfig` (if not already exists)
2. Add entry to `HazardConfigs` array

**Step 2: Configure Hazard Entry**
```
Name: "Starvation"
Hazard Tag: "StatusEffect.Survival.Starvation"
Effect Class: UNomadSurvivalStatusEffect
DoT Percent: 0.005 (0.5% of max health per second)
Stat Type: "HUNGER"
UI Type: "BAR"
Gameplay: "Deals health damage when hunger reaches 0"
Visual Cue: "Screen desaturation, stomach rumble sounds, weakness effects"
Designer Notes: "Should encourage eating but not be instantly lethal"
```

**Step 3: Create Corresponding Effect Config**
1. Create `NomadInfiniteEffectConfig` for the actual effect
2. Name it `StarvationStatusEffect`

**Step 4: Configure Status Effect**
```
Effect Name: "Starving"
Description: "Severe hunger is damaging your health"
Effect Tag: "StatusEffect.Survival.Starvation" (must match hazard config)
Category: Negative
Icon: [Hunger/starvation icon]

Application Mode: Both (uses DoT from hazard config + attribute modifiers)

Persistent Attribute Modifier:
- "RPG.Attributes.MovementSpeed": 0.8 (20% speed reduction from weakness)

Blocking Tags:
- "Status.Block.Sprint" (too weak to sprint when starving)
```

**Step 5: Configure Feedback**
```
Visual Effects:
- Loop Effect: "VFX_Starvation_Weakness" (screen desaturation, vignette)

Audio Cues:
- Apply Sound: "SFX_Stomach_Rumble_Severe"
- Tick Sound: "SFX_Heartbeat_Weak" (plays with damage ticks)
- Loop Sound: "SFX_Hunger_Ambience" (ongoing audio)
```

**Step 6: Test Survival Integration**
```
Debug console commands:
survival.sethunger 0     (trigger starvation)
survival.sethunger 50    (restore some hunger to stop effect)
survival.info            (show current survival stats)
```

---

## Configuration Examples

### Damage Over Time Effects

#### Poison (Medium Duration, Moderate Damage)
```
Type: NomadTimedEffectConfig
Duration: 20.0 seconds
Tick Interval: 2.0 seconds (damage every 2 seconds)
Application Mode: DamageEvent
Damage Type: PoisonDamageType
Stat Modifiers Per Tick:
  "RPG.Statistics.Health": -5.0 (5 HP every 2 seconds = 2.5 HP/sec)
Visual: Green particle effects, screen green tint
Audio: Gurgling sounds, labored breathing
```

#### Fire Damage (Short Duration, High Damage)
```
Type: NomadTimedEffectConfig  
Duration: 8.0 seconds
Tick Interval: 0.5 seconds (damage twice per second)
Application Mode: DamageEvent
Damage Type: FireDamageType
Stat Modifiers Per Tick:
  "RPG.Statistics.Health": -3.0 (6 HP per second)
Visual: Fire particles, orange screen overlay
Audio: Crackling fire, pain sounds
```

#### Disease (Long Duration, Attribute Penalties)
```
Type: NomadTimedEffectConfig
Duration: 300.0 seconds (5 minutes)
Tick Interval: 10.0 seconds
Application Mode: StatModification
Persistent Attribute Modifier:
  "RPG.Attributes.MovementSpeed": 0.9 (10% speed reduction)
  "RPG.Attributes.StaminaRegen": 0.7 (30% slower stamina regen)
  "RPG.Statistics.MaxStamina": 0.8 (20% reduced max stamina)
Visual: Sickly green aura, coughing animation triggers
Audio: Coughing, wheezing sounds
```

### Healing and Buff Effects

#### Healing Potion (Instant)
```
Type: NomadInstantEffectConfig
Application Mode: StatModification
Stat Modifiers:
  "RPG.Statistics.Health": +50.0 (restore 50 HP immediately)
Visual: Golden sparkles, bright flash
Audio: Magical chime, refreshing sound
Category: Positive
```

#### Stamina Regeneration Boost (Timed)
```
Type: NomadTimedEffectConfig
Duration: 60.0 seconds
Application Mode: StatModification
Persistent Attribute Modifier:
  "RPG.Attributes.StaminaRegen": 2.0 (double stamina regeneration rate)
Visual: Blue energy aura around character
Audio: Energizing hum, power-up sound
Category: Positive
```

#### Strength Enhancement (Timed)
```
Type: NomadTimedEffectConfig
Duration: 45.0 seconds
Application Mode: StatModification
Persistent Attribute Modifier:
  "RPG.Attributes.AttackDamage": 1.3 (30% damage increase)
  "RPG.Attributes.CarryCapacity": 1.5 (50% more carrying capacity)
Visual: Red aura, muscle definition enhancement
Audio: Power surge sound, strength vocalization
Category: Positive
```

### Environmental Effects

#### Extreme Cold (Survival)
```
Type: NomadSurvivalHazardConfig entry
Hazard Tag: "StatusEffect.Survival.Hypothermia"
DoT Percent: 0.003 (0.3% health per second)
Effect Config (NomadInfiniteEffectConfig):
  Persistent Attribute Modifier:
    "RPG.Attributes.MovementSpeed": 0.7 (30% speed reduction)
    "RPG.Attributes.StaminaDecayRate": 1.4 (40% faster stamina drain)
  Blocking Tags:
    "Status.Block.Sprint" (too cold to sprint)
Visual: Blue screen tint, shivering animation, breath vapor
Audio: Chattering teeth, wind sounds, labored breathing
```

#### Extreme Heat (Survival)
```  
Type: NomadSurvivalHazardConfig entry
Hazard Tag: "StatusEffect.Survival.Heatstroke"
DoT Percent: 0.0 (no direct health damage)
Effect Config (NomadInfiniteEffectConfig):
  Persistent Attribute Modifier:
    "RPG.Attributes.ThirstDecayRate": 4.0 (4x thirst consumption)
    "RPG.Attributes.StaminaDecayRate": 1.5 (50% faster stamina drain)
  Blocking Tags:
    "Status.Block.Sprint" (too hot to sprint safely)
Visual: Heat shimmer, sweat effects, red screen tint
Audio: Heavy breathing, heat ambience, heartbeat
```

#### Radiation Exposure (Environmental)
```
Type: NomadTimedEffectConfig
Duration: 600.0 seconds (10 minutes)
Tick Interval: 5.0 seconds
Application Mode: DamageEvent  
Damage Type: RadiationDamageType
Stat Modifiers Per Tick:
  "RPG.Statistics.Health": -2.0 (gradual health loss)
Persistent Attribute Modifier:
  "RPG.Attributes.HealthRegen": 0.5 (50% slower health regeneration)
Visual: Green glow, Geiger counter UI element, static overlay
Audio: Geiger counter clicks, electronic interference
```

### Equipment and Item Effects

#### Lightweight Gear (Equipment)
```
Type: NomadInfiniteEffectConfig
Persistent Attribute Modifier:
  "RPG.Attributes.MovementSpeed": 1.15 (15% speed increase)
  "RPG.Attributes.StaminaDecayRate": 0.9 (10% slower stamina drain)
  "RPG.Statistics.ArmorRating": 0.8 (20% less armor protection)
Visual: Subtle blue glow on equipment, lighter footstep effects
Audio: Quieter footsteps, wind-like movement sounds
Category: Neutral (trade-off effect)
```

#### Magic Ring of Protection (Equipment)
```
Type: NomadInfiniteEffectConfig
Persistent Attribute Modifier:
  "RPG.Statistics.ArmorRating": 1.2 (20% more protection)
  "RPG.Attributes.MagicResistance": 1.3 (30% magic resistance)
  "RPG.Statistics.HealthRegen": 1.1 (10% faster health regeneration)
Visual: Protective aura shimmer, magical sparkles
Audio: Subtle magical hum, protective ward sounds
Category: Positive
```

#### Cursed Weapon (Equipment)
```
Type: NomadInfiniteEffectConfig
Persistent Attribute Modifier:
  "RPG.Attributes.AttackDamage": 1.4 (40% damage increase)
  "RPG.Statistics.HealthRegen": 0.0 (no health regeneration)
  "RPG.Attributes.MagicResistance": 0.7 (30% less magic resistance)
Visual: Dark aura, occasionally red glow, shadowy effects
Audio: Whispers, ominous humming, curse ambience
Category: Negative (powerful but cursed)
```

---

## Asset Requirements

### Required Assets for Each Effect

#### Textures
- **Icon**: 64x64 or 128x128 UI icon for status bars
- **Status Bar**: Optional background texture for status display
- **Screen Overlays**: Optional fullscreen textures for visual feedback

#### Audio Assets
- **Apply Sound**: Played when effect starts
- **Tick Sound**: Played on each damage/heal tick (optional)
- **Loop Sound**: Ambient sound while effect is active (optional)
- **Remove Sound**: Played when effect ends

#### Visual Effects (Niagara/Particles)
- **Start Effect**: Particle system when effect begins
- **Loop Effect**: Ongoing particle system while active
- **End Effect**: Particle system when effect concludes
- **Screen Effects**: Post-process materials for screen distortion

### Asset Naming Conventions

#### Icons
```
Icon_StatusEffect_[Category]_[Name]
Examples:
- Icon_StatusEffect_Positive_Healing
- Icon_StatusEffect_Negative_Poison  
- Icon_StatusEffect_Survival_Starvation
- Icon_StatusEffect_Equipment_HeavyArmor
```

#### Audio
```
SFX_StatusEffect_[Name]_[Type]
Examples:
- SFX_StatusEffect_Poison_Apply
- SFX_StatusEffect_Poison_Tick
- SFX_StatusEffect_Poison_Loop
- SFX_StatusEffect_Healing_Apply
```

#### Visual Effects
```  
VFX_StatusEffect_[Name]_[Type]
Examples:
- VFX_StatusEffect_Fire_Start
- VFX_StatusEffect_Fire_Loop
- VFX_StatusEffect_Fire_End
- VFX_StatusEffect_Healing_Burst
```

#### Materials/Post-Process
```
M_StatusEffect_[Name]_[Type]
Examples:
- M_StatusEffect_Poison_ScreenTint
- M_StatusEffect_Starvation_Desaturation
- M_StatusEffect_Fire_ScreenDistortion
```

### Asset Organization

#### Folder Structure
```
Content/
├── StatusEffects/
│   ├── Configs/
│   │   ├── Instant/
│   │   ├── Timed/ 
│   │   ├── Infinite/
│   │   └── Survival/
│   ├── Audio/
│   │   ├── Apply/
│   │   ├── Tick/
│   │   ├── Loop/
│   │   └── Remove/
│   ├── VFX/
│   │   ├── Particles/
│   │   └── Materials/
│   └── UI/
│       ├── Icons/
│       └── Textures/
```

### Quality Standards

#### Icons
- **Resolution**: 128x128 minimum, 256x256 preferred
- **Format**: PNG with transparency
- **Style**: Consistent with game's UI art style
- **Clarity**: Recognizable at small sizes (32x32)

#### Audio
- **Format**: WAV or OGG Vorbis
- **Length**: Apply/Remove sounds 0.5-2 seconds, Loop sounds 2-10 seconds
- **Volume**: Consistent levels, not overpowering
- **Quality**: 44.1kHz sample rate minimum

#### Visual Effects
- **Performance**: Target 60 FPS on minimum spec hardware
- **Scalability**: Multiple LOD levels for different quality settings
- **Style**: Consistent with game's visual style
- **Clarity**: Clear indication of effect type and intensity

---

## UI Integration

### Status Effect Display

#### Status Bar Integration
```
The UI system automatically displays active effects using:
- Effect Name: From DataAsset configuration
- Icon: From DataAsset icon property
- Duration: For timed effects (countdown timer)
- Stacks: For stackable effects (stack counter)
- Category Color: Green (Positive), Red (Negative), Yellow (Neutral)
```

#### Tooltip Information
```
When players hover over status effect icons, tooltips show:
- Effect Name and Description from DataAsset
- Remaining duration (for timed effects)
- Current effect magnitude (damage per second, etc.)
- Stack count (for stackable effects)
```

### Notification System

#### Effect Applied Notifications
```
When effects are applied, the notification system shows:
- Effect icon and name
- Brief description of what happened
- Positive/negative indicator (color and icon)
- Duration information (for temporary effects)
```

#### Effect Expired Notifications
```
When effects end naturally:
- "Effect Expired" message with effect name
- Icon fades out with animation
- Audio feedback (if configured)
```

### Health Bar Integration

#### Damage Over Time Indicators
```
Health bars show special indicators for DoT effects:
- Red blinking border for health-damaging effects
- Damage preview (how much health will be lost)
- Time until next damage tick
- Total damage remaining (for finite effects)
```

#### Healing Over Time Indicators
```
Health bars show special indicators for HoT effects:
- Green glowing border for health-restoring effects
- Healing preview (how much health will be gained)
- Time until next healing tick
- Total healing remaining (for finite effects)
```

### UI Configuration Options

#### Designer Controls
```
Each effect config provides UI customization:
- Show in Status Bar: Whether to display in status effects area
- Show Notifications: Whether to show apply/remove notifications
- Show Tooltips: Whether to provide detailed tooltip information
- Priority Level: Display order in status bar (higher priority shows first)
```

#### Visual Customization
```
UI elements can be customized per effect:
- Background Color: Custom background for this effect's UI elements
- Border Style: Special border treatments for important effects
- Animation Style: How the effect appears/disappears in UI
- Sound Overrides: Custom UI sounds for this specific effect
```

---

## Testing and Validation

### Built-in Validation

#### Configuration Validation
```
The system automatically validates your configurations:
- Required fields must be filled (Effect Name, Tag, etc.)
- Numeric values must be within reasonable ranges
- Asset references must be valid
- Tags must follow naming conventions
```

#### Runtime Validation
```
During gameplay, the system checks:
- Effect applications don't conflict with existing effects
- Stat modifications don't cause invalid values
- Audio/visual assets load correctly
- UI elements display properly
```

### Testing Tools

#### Console Commands
```
se.apply [EffectTag]           - Apply effect by tag
se.remove [EffectTag]          - Remove effect by tag  
se.list                        - List all active effects
se.clear                       - Remove all effects
se.info [EffectTag]           - Show detailed effect information
se.test [EffectTag] [Duration] - Apply effect for testing with custom duration
```

#### Debug Display
```
showdebug statuseffects        - Show debug overlay with active effects
showdebug survival             - Show survival stats and thresholds
stat statuseffects             - Show performance statistics
```

#### Blueprint Testing Functions
```
The system provides Blueprint-callable testing functions:
- TestStatusEffectApplication: Verify effects apply correctly
- TestStatusEffectStacking: Test stacking behavior
- TestStatusEffectDuration: Verify timing and duration
- TestStatusEffectRemoval: Test cleanup and removal
```

### Testing Workflow

#### Phase 1: Isolated Testing
1. **Single Effect Testing**: Test each effect individually
2. **Value Verification**: Confirm stat changes match configuration
3. **Audio/Visual Check**: Verify feedback plays correctly
4. **UI Integration**: Check status bar and notifications display properly

#### Phase 2: Integration Testing  
1. **Multiple Effects**: Test with several effects active simultaneously
2. **Stacking Behavior**: Test effects that can stack
3. **Conflicting Effects**: Test effects that modify same stats
4. **Performance Impact**: Monitor FPS with many effects active

#### Phase 3: Gameplay Testing
1. **Natural Application**: Test effects in normal gameplay scenarios
2. **Player Experience**: Get feedback on feel and balance
3. **Edge Cases**: Test unusual situations (death while affected, save/load, etc.)
4. **Multiplayer**: Test replication and synchronization

### Common Validation Issues

#### Configuration Errors
```
ERROR: Effect tag is empty or invalid
FIX: Set unique, hierarchical tag like "StatusEffect.Category.Name"

ERROR: Effect name is empty
FIX: Provide user-friendly name for UI display

ERROR: Invalid attribute tag in modifiers
FIX: Use correct attribute tags like "RPG.Attributes.MovementSpeed"

ERROR: Damage type class not set for DamageEvent mode
FIX: Assign appropriate UDamageType subclass
```

#### Runtime Errors
```
ERROR: Effect config asset failed to load
FIX: Check asset path and ensure it's not corrupted

ERROR: Audio asset not found
FIX: Verify audio file exists and is properly imported

ERROR: Visual effect spawning failed  
FIX: Check particle system is valid and has required modules

ERROR: Attribute modification failed
FIX: Ensure target has ARSStatisticsComponent with required attributes
```

---

## Common Scenarios

### Scenario 1: Creating Equipment Set Bonuses

**Challenge**: Player wears multiple pieces of equipment, want bonus when wearing full set

**Solution**: 
1. Create individual equipment effects for each piece (using NomadInfiniteEffectConfig)
2. Create separate "Set Bonus" effect that checks for all pieces
3. Equipment system applies/removes set bonus based on equipped items

**Configuration Example**:
```
// Individual piece: Knight's Helmet
Effect Tag: "StatusEffect.Equipment.KnightHelmet"
Persistent Attribute Modifier:
  "RPG.Statistics.ArmorRating": 1.1 (10% armor increase)

// Set bonus: Full Knight Set (2+ pieces)
Effect Tag: "StatusEffect.Equipment.KnightSetBonus"  
Persistent Attribute Modifier:
  "RPG.Attributes.HealthRegen": 1.5 (50% health regen bonus)
  "RPG.Statistics.ArmorRating": 1.2 (Additional 20% armor)
```

### Scenario 2: Environmental Zone Effects

**Challenge**: Different areas have different environmental effects (swamp poison, arctic cold)

**Solution**:
1. Create infinite effects for each environmental condition
2. Use trigger volumes to apply/remove effects when entering/leaving areas
3. Configure effects to automatically remove when leaving trigger

**Configuration Example**:
```
// Swamp Poison Zone
Effect Tag: "StatusEffect.Environment.SwampPoison"
Type: NomadInfiniteEffectConfig
Application Mode: DamageEvent
Damage Type: PoisonDamageType
Tick Interval: 3.0 seconds
Stat Modifiers Per Tick:
  "RPG.Statistics.Health": -2.0 (2 HP every 3 seconds)
Visual: Green fog particles, sickly screen tint
Audio: Bubbling swamp sounds, labored breathing
```

### Scenario 3: Crafted Consumables with Scaling

**Challenge**: Consumables should have different potency based on crafting quality/materials

**Solution**:
1. Create multiple effect configs for different potency levels  
2. Crafting system chooses appropriate config based on materials/skill
3. Use consistent naming scheme for easy management

**Configuration Example**:
```
// Basic Healing Potion
Effect Tag: "StatusEffect.Potion.Healing.Basic"
Stat Modifiers: "RPG.Statistics.Health": +30.0

// Quality Healing Potion  
Effect Tag: "StatusEffect.Potion.Healing.Quality"
Stat Modifiers: "RPG.Statistics.Health": +50.0

// Master Healing Potion
Effect Tag: "StatusEffect.Potion.Healing.Master"  
Stat Modifiers: "RPG.Statistics.Health": +80.0
Duration: 10.0 seconds (also provides regeneration)
Stat Modifiers Per Tick: "RPG.Statistics.Health": +2.0
```

### Scenario 4: Conditional Effect Stacking

**Challenge**: Some effects should stack, others shouldn't, some should refresh duration

**Solution**:
1. Configure stacking behavior per effect in DataAsset
2. Use effect tags to control interaction between related effects
3. Set up removal/refresh logic in effect configs

**Configuration Examples**:
```
// Bleeding - Should Stack (multiple wounds)
Can Stack: true
Max Stacks: 5
Stacking Behavior: AddStack (each application adds new stack)

// Speed Boost - Should Refresh (drinking potion again refreshes duration)
Can Stack: false  
Stacking Behavior: RefreshDuration

// Armor Spell - Should Replace (new cast replaces old)
Can Stack: false
Stacking Behavior: Replace
```

### Scenario 5: Complex Survival Interactions

**Challenge**: Temperature affects thirst rate, hunger affects movement, etc.

**Solution**:
1. Create survival effects that modify multiple attributes
2. Use attribute system's multiplier stacking for natural interactions
3. Configure effects to modify consumption rates, not just direct stats

**Configuration Example**:
```
// Extreme Heat (Heatstroke)
Effect Tag: "StatusEffect.Survival.Heatstroke"
Persistent Attribute Modifier:
  "RPG.Attributes.ThirstDecayRate": 3.0 (3x thirst consumption)
  "RPG.Attributes.SweatRate": 2.0 (2x sweat production)  
  "RPG.Attributes.StaminaDecayRate": 1.4 (40% faster stamina drain)
Blocking Tags:
  "Status.Block.Sprint" (can't sprint safely in extreme heat)

// Starvation (Severe Hunger)
Effect Tag: "StatusEffect.Survival.Starvation"
DoT Percent: 0.005 (0.5% health per second)
Persistent Attribute Modifier:
  "RPG.Attributes.MovementSpeed": 0.8 (20% speed reduction from weakness)
  "RPG.Attributes.StaminaRegen": 0.6 (40% slower stamina recovery)
  "RPG.Attributes.CarryCapacity": 0.9 (10% less carrying capacity)
```

---

## Troubleshooting

### Effect Not Applying

#### Symptom: Console command succeeds but effect doesn't appear
**Possible Causes**:
- Effect config asset is invalid or corrupted
- Required components missing on target character
- Effect tag conflicts with existing effect

**Debugging Steps**:
1. Check console for error messages
2. Verify config asset loads correctly: `se.info [EffectTag]`
3. Ensure target has UNomadStatusEffectManagerComponent
4. Check if effect is being immediately removed by other systems

**Solutions**:
- Fix configuration validation errors
- Add required components to character Blueprint
- Use unique effect tags to avoid conflicts
- Check for competing systems that might remove effects

### Visual/Audio Effects Not Playing

#### Symptom: Effect applies but no visual or audio feedback
**Possible Causes**:
- Asset references are broken or null
- Assets failed to load due to missing files
- Audio/visual components not present on character

**Debugging Steps**:
1. Check asset references in DataAsset configuration
2. Verify referenced assets exist and are not corrupted
3. Test assets individually outside of status effect system
4. Check character has required components (audio, particle systems)

**Solutions**:
- Fix broken asset references in configuration
- Re-import corrupted or missing audio/visual assets
- Add required components to character Blueprint
- Use placeholder assets during testing

### Performance Issues

#### Symptom: Game slows down with many status effects
**Possible Causes**:
- Too many effects ticking too frequently
- Heavy visual effects with many particles
- Inefficient audio streaming or looping

**Debugging Steps**:
1. Use `stat statuseffects` to see performance metrics
2. Check tick intervals in effect configurations
3. Profile visual effects using UE profiling tools
4. Monitor audio streaming performance

**Solutions**:
- Increase tick intervals for non-critical effects
- Optimize particle systems and reduce particle counts
- Use LOD systems for visual effects
- Implement effect pooling for frequently used effects

### UI Display Issues

#### Symptom: Effects not showing in status bar or showing incorrectly
**Possible Causes**:
- UI components not properly configured
- Effect category or priority settings incorrect
- Icon assets missing or wrong format

**Debugging Steps**:
1. Check UI widget Blueprint configuration
2. Verify effect category and priority settings
3. Test icon assets directly in UI
4. Check for UI update notifications being sent

**Solutions**:
- Configure UI widgets to listen for status effect events
- Set appropriate category and priority in effect configs
- Use correct icon asset format and resolution
- Ensure UI notification system is functioning

### Multiplayer Synchronization

#### Symptom: Effects not replicating correctly to clients
**Possible Causes**:
- Effects applied on client instead of server
- Replication component configuration issues
- Network relevancy filtering problems

**Debugging Steps**:
1. Check if effects are being applied with authority
2. Verify UNomadStatusEffectManagerComponent replication settings
3. Test with `showdebug net` to see replication traffic
4. Check client logs for replication errors

**Solutions**:
- Ensure effects are applied on server (authority)
- Configure proper replication settings on manager component
- Adjust network relevancy settings for better coverage
- Add client prediction for immediate feedback

### Configuration Validation Failures

#### Symptom: DataAsset shows validation errors in editor
**Common Errors and Solutions**:

```
"Effect tag is empty"
→ Set valid tag: "StatusEffect.Category.EffectName"

"Effect name is empty"  
→ Provide user-friendly name for UI

"Invalid attribute modifier"
→ Use correct format: "RPG.Attributes.AttributeName"

"Damage type required for damage events"
→ Set DamageTypeClass when using DamageEvent mode

"Duration must be positive for timed effects"
→ Set Duration > 0 for UNomadTimedEffectConfig

"Stack limit must be at least 1"
→ Set MaxStacks ≥ 1 when CanStack = true
```

### Save/Load Issues

#### Symptom: Effects lost or corrupted after save/load
**Possible Causes**:
- Status effect manager not properly serialized
- Effect config assets changed since save was made
- Infinite effects not persisting correctly

**Debugging Steps**:
1. Check save game data structure includes status effects
2. Verify config assets haven't been moved or renamed
3. Test save/load with simple effects first
4. Check for version compatibility issues

**Solutions**:
- Include status effect manager in save game serialization
- Maintain backward compatibility when changing configs
- Implement graceful fallbacks for missing assets
- Add save game version checking and migration

---

## Best Practices

### Design Principles

#### 1. Player Communication
- **Clear Feedback**: Every effect should have obvious visual/audio indicators
- **Understandable Names**: Use terminology players immediately understand
- **Consistent Iconography**: Similar effects should have similar visual styles
- **Meaningful Duration**: Effect timing should feel appropriate to its purpose

#### 2. Gameplay Balance
- **Gradual Escalation**: Start with mild effects, escalate to severe consequences
- **Player Agency**: Always provide ways for players to mitigate or remove effects
- **Risk vs Reward**: Negative effects should have proportional benefits elsewhere
- **Accessibility**: Consider colorblind players and audio-only feedback options

#### 3. Technical Consistency
- **Naming Conventions**: Follow consistent tag and asset naming patterns
- **Performance Mindful**: Consider performance impact of visual/audio effects
- **Modular Design**: Create reusable components that work across different effects
- **Documentation**: Comment your configurations for future maintenance

### Configuration Guidelines

#### Effect Tags
```
Follow hierarchical naming:
✓ "StatusEffect.Survival.Starvation"
✓ "StatusEffect.Equipment.HeavyArmor"  
✓ "StatusEffect.Potion.Healing.Basic"

Avoid flat naming:
✗ "Starvation"
✗ "HeavyArmor"
✗ "HealingPotion"
```

#### Attribute Modifiers
```
Use multipliers, not additions:
✓ MovementSpeed: 0.8 (20% reduction)
✓ AttackDamage: 1.25 (25% increase)

Avoid absolute values:
✗ MovementSpeed: -50.0 (unclear base value)
✗ AttackDamage: +10.0 (doesn't scale with character level)
```

#### Duration and Timing
```
Use intuitive intervals:
✓ Tick Interval: 1.0 (once per second - easy to understand)
✓ Duration: 30.0 (30 seconds - round number)

Avoid fractional timing:
✗ Tick Interval: 0.73 (confusing to debug)
✗ Duration: 17.3 (hard to predict when it ends)
```

### Asset Organization

#### Folder Structure
```
Organize by effect type, not asset type:
✓ StatusEffects/Survival/Starvation/
    ├── StarvationConfig.uasset
    ├── Icon_Starvation.png
    ├── VFX_Starvation_Loop.uasset
    └── SFX_Starvation_Apply.wav

Avoid asset-type organization for status effects:
✗ Audio/StatusEffects/
✗ VFX/StatusEffects/  
✗ UI/StatusEffects/
```

#### Version Control
```
Always commit related assets together:
- Config DataAsset
- Referenced audio/visual assets
- Updated documentation
- Test configurations

Use descriptive commit messages:
"Add starvation effect with screen desaturation and stomach rumble audio"
```

### Testing Strategy

#### Incremental Testing
1. **Config Validation**: Test configuration before applying to character
2. **Isolated Testing**: Test each effect individually first
3. **Integration Testing**: Test with other effects and systems
4. **Player Testing**: Get feedback from actual players

#### Edge Case Testing
```
Test unusual scenarios:
- Applying effect to dead character
- Saving/loading while effect is active
- Removing character components while effect runs
- Network disconnection during effect application
- Multiple identical effects applied simultaneously
```

### Common Mistakes to Avoid

#### Configuration Mistakes
```
❌ Using absolute stat values instead of multipliers
❌ Forgetting to set effect categories for UI color coding
❌ Creating effects that conflict with core game mechanics
❌ Using extremely short or long durations without playtesting
❌ Overusing blocking tags that frustrate players
```

#### Asset Mistakes
```
❌ Using inconsistent icon styles across related effects
❌ Audio effects that are too loud or too quiet
❌ Visual effects that obscure important gameplay elements
❌ Large file sizes for assets that trigger frequently
❌ Broken asset references that cause runtime errors
```

#### Gameplay Mistakes
```
❌ Effects that feel punitive without player agency
❌ Unclear visual feedback that confuses players
❌ Stacking effects that create impossible situations
❌ Effects that bypass player skill and strategy
❌ Inconsistent rules between similar effects
```

### Quality Assurance Checklist

#### Before Submitting Effects
- [ ] Configuration passes all validation checks
- [ ] Effect applies and removes correctly
- [ ] Visual effects display appropriately
- [ ] Audio effects play at correct volume and timing
- [ ] UI integration works (status bar, tooltips, notifications)
- [ ] Effect works in multiplayer environments
- [ ] Performance impact is acceptable
- [ ] Save/load preserves effect state correctly
- [ ] Effect interacts properly with related systems
- [ ] Documentation is updated with new effect details

#### Playtesting Validation
- [ ] Effect purpose is immediately clear to players
- [ ] Visual/audio feedback feels appropriate
- [ ] Duration feels right for the gameplay impact
- [ ] Players understand how to counter or mitigate effect
- [ ] Effect doesn't break player expectations
- [ ] Effect enhances rather than detracts from gameplay
- [ ] New players can understand the effect without explanation
- [ ] Effect works well across different character builds/playstyles

---

This Designer Configuration Guide provides everything you need to create compelling status effects for the Nomad Survival System. For technical implementation details, see the [Developer Guide](NomadSurvivalSystem_Developer_Guide.md). For quick reference, check the [Quick Reference Guide](NomadSurvivalSystem_QuickReference.md).