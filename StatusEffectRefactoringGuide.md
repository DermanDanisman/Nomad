# Status Effect System Refactoring Guide

## Overview

This guide explains the refactoring of the `NomadStatusEffectGameplayHelpers` system to eliminate hardcoded attribute tags and make the entire system fully data-driven through existing status effects and their configurations.

## Key Architecture Change

**Previous Approach**: Created dedicated movement speed status effect classes
**Current Approach**: Use existing status effect types (Infinite, Timed, Survival) with configurable data assets

The system now leverages the existing, well-designed status effect architecture where:
- **Movement speed modifications** are handled via `PersistentAttributeModifier` in config assets
- **Input blocking** is handled via `BlockingTags` in config assets
- **Effect types** are determined by base class choice (Infinite, Timed, Survival)

## Key Changes

### 1. Configurable Movement Speed System

**Old Approach** (Hardcoded):
```cpp
// Hardcoded attribute tag usage
UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromStat(Character);
UNomadStatusEffectGameplayHelpers::ApplyMovementSpeedModifierToState(MoveComp, State, 0.5f, MyGuid);
```

**New Approach** (Configurable):
```cpp
// Configurable attribute tag
UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromAttribute(Character, CustomMovementSpeedTag);
// Or use the recommended default
UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromDefaultAttribute(Character);

// Status effect-driven movement modification using existing status effect types
UNomadStatusEffectGameplayHelpers::ApplyMovementSpeedStatusEffect(
    Character, UNomadInfiniteStatusEffect::StaticClass(), Duration);
```

### 2. Data-Driven Movement Effects

**Use existing status effect types with proper configuration:**

- **UNomadInfiniteStatusEffect**: For permanent movement changes (equipment bonuses, buffs)
- **UNomadTimedStatusEffect**: For temporary movement changes (potions, debuffs) 
- **UNomadSurvivalStatusEffect**: For survival-related movement effects (starvation, dehydration)

**Configuration via Data Assets:**
```cpp
// In your UNomadInfiniteEffectConfig asset:
PersistentAttributeModifier.MovementSpeed = 0.8f;  // 20% speed reduction
BlockingTags.AddTag("Status.Block.Sprint");        // Block sprinting
BlockingTags.AddTag("Status.Block.Jump");          // Block jumping
```

### 3. Enhanced Input Blocking System

**Old** (Limited to sprint):
```cpp
bool bBlocked = UNomadStatusEffectGameplayHelpers::IsSprintBlocked(Character);
```

**New** (Any action):
```cpp
bool bSprintBlocked = UNomadStatusEffectGameplayHelpers::IsSprintBlocked(Character);
bool bJumpBlocked = UNomadStatusEffectGameplayHelpers::IsJumpBlocked(Character);
bool bCustomBlocked = UNomadStatusEffectGameplayHelpers::IsActionBlocked(
    Character, FGameplayTag::RequestGameplayTag("Status.Block.Interact"));
```

## Migration Steps

### For Existing Movement Speed Code

1. **Replace hardcoded movement sync:**
   ```cpp
   // Old
   UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromStat(Character);
   
   // New
   UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromDefaultAttribute(Character);
   ```

2. **Replace direct movement speed modification:**
   ```cpp
   // Old
   UNomadStatusEffectGameplayHelpers::ApplyMovementSpeedModifierToState(
       MoveComp, ELocomotionState::EWalk, 0.5f, MyGuid);
   
   // New - Use status effects with config assets
   UNomadStatusEffectGameplayHelpers::ApplyMovementSpeedStatusEffect(
       Character, UNomadTimedStatusEffect::StaticClass(), Duration);
   ```

3. **Enhance input blocking:**
   ```cpp
   // Old
   if (UNomadStatusEffectGameplayHelpers::IsSprintBlocked(Character))
   
   // New - More options available
   if (UNomadStatusEffectGameplayHelpers::IsJumpBlocked(Character) ||
       UNomadStatusEffectGameplayHelpers::IsActionBlocked(Character, CustomTag))
   ```

### For New Movement Effects

1. **Create effect config assets** using appropriate config type:
   - `UNomadInfiniteEffectConfig` for permanent effects
   - `UNomadTimedEffectConfig` for temporary effects
   - Survival-specific configs for survival effects

2. **Configure movement modifications:**
   ```cpp
   // In your config asset:
   PersistentAttributeModifier.RPG_Attributes_MovementSpeed = 1.5f;  // 50% speed boost
   ```

3. **Configure input blocking:**
   ```cpp
   // In your config asset:
   BlockingTags.AddTag("Status.Block.Sprint");
   BlockingTags.AddTag("Status.Block.Jump");
   ```

4. **Use existing status effect classes** as base classes:
   ```cpp
   // Apply infinite movement speed boost
   SEManager->ApplyInfiniteStatusEffect(UNomadInfiniteStatusEffect::StaticClass());
   
   // Apply timed movement penalty
   SEManager->ApplyTimedStatusEffect(UNomadTimedStatusEffect::StaticClass(), 30.0f);
   ```

## Configuration Examples

### Speed Boost Effect (Equipment Bonus)
```cpp
// UNomadInfiniteEffectConfig asset setup:
EffectName = "Speed Boost";
EffectTag = "StatusEffect.Equipment.SpeedBoost";
PersistentAttributeModifier.RPG_Attributes_MovementSpeed = 1.25f;  // 25% speed increase
Category = ENomadStatusCategory::Positive;
```

### Movement Penalty (Survival Effect)
```cpp
// UNomadInfiniteEffectConfig asset setup:
EffectName = "Exhaustion";
EffectTag = "StatusEffect.Survival.MovementPenalty.Heavy";
PersistentAttributeModifier.RPG_Attributes_MovementSpeed = 0.7f;   // 30% speed reduction
BlockingTags.AddTag("Status.Block.Sprint");                       // Can't sprint when exhausted
Category = ENomadStatusCategory::Negative;
```

### Temporary Slow Effect (Potion/Debuff)
```cpp
// UNomadTimedEffectConfig asset setup:
EffectName = "Slowness";
EffectTag = "StatusEffect.Debuff.Slowness";
PersistentAttributeModifier.RPG_Attributes_MovementSpeed = 0.5f;   // 50% speed reduction
BlockingTags.AddTag("Status.Block.Sprint");
BlockingTags.AddTag("Status.Block.Jump");
Duration = 15.0f;  // 15 seconds
Category = ENomadStatusCategory::Negative;
```

## Backward Compatibility

- All existing methods are maintained with deprecation warnings
- Gradual migration path available
- New recommended methods provided alongside deprecated ones
- No breaking changes for existing functionality

## Benefits

1. **Simpler Architecture** - Fewer classes to maintain, leveraging existing system
2. **More Flexible** - Any status effect can modify movement speed via config
3. **Consistent** - All effects use the same configuration pattern
4. **Data-Driven Design** - All values come from config assets
5. **Flexible Input Blocking** - Support for any action blocking via gameplay tags
6. **Unified API** - Consistent interface for all movement speed modifications
7. **Better Integration** - Seamless integration with existing survival system
8. **Enhanced Debugging** - Helper methods for checking active effects
9. **Maintainability** - No hardcoded values scattered throughout the codebase

## Testing Checklist

- [ ] Movement speed changes work through status effects
- [ ] Input blocking works for sprint, jump, and custom actions
- [ ] Survival mechanics function correctly with new system
- [ ] Multiplayer synchronization works properly
- [ ] UI updates reflect status effect changes
- [ ] Backward compatibility maintained for existing code
- [ ] Performance impact is minimal or improved

## Common Pitfalls

1. **Forgetting to sync movement speed** after applying/removing effects
2. **Not setting proper BlockingTags** in config assets
3. **Mixing old and new approaches** in the same codebase
4. **Not configuring PersistentAttributeModifier** for movement effects
5. **Using hardcoded tags** instead of configurable ones
6. **Creating unnecessary specialized classes** instead of using config-driven existing classes

## Support

For questions about migration or new functionality, refer to:
- Method documentation in `NomadStatusEffectGameplayHelpers.h`
- Status effect class documentation
- Configuration asset documentation for each status effect type