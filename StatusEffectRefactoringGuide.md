# Status Effect System Migration Guide

This document outlines the refactoring of the Nomad status effect system to eliminate hardcoded attribute tags and direct stat manipulations, making the system fully data-driven through status effects and their configurations.

## Overview

The previous system relied on hardcoded attribute tags and direct stat value manipulations. The new system uses exclusively status effects and their associated configurations for all survival mechanics, including movement speed adjustments and input blocking.

## Key Changes

### 1. Movement Speed System Refactoring

#### Old Approach (Deprecated)
```cpp
// Hardcoded attribute tag
UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromStat(MyCharacter);

// Manual GUID tracking
UNomadStatusEffectGameplayHelpers::ApplyMovementSpeedModifierToState(
    MoveComp, ELocomotionState::EWalk, 0.5f, MyGuid);
```

#### New Approach (Recommended)
```cpp
// Configurable attribute tag
UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromAttribute(
    MyCharacter, MyCustomMovementSpeedTag);

// Or use the default
UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromDefaultAttribute(MyCharacter);

// Status effect-driven movement modification
UNomadStatusEffectGameplayHelpers::ApplyMovementSpeedStatusEffect(
    MyCharacter, UNomadSpeedPenaltyStatusEffect::StaticClass(), 0.0f);
```

### 2. Input Blocking Enhancement

#### Old Approach (Limited)
```cpp
// Only sprint blocking was supported
bool bIsBlocked = UNomadStatusEffectGameplayHelpers::IsSprintBlocked(MyCharacter);
```

#### New Approach (Flexible)
```cpp
// Multiple action blocking support
bool bSprintBlocked = UNomadStatusEffectGameplayHelpers::IsSprintBlocked(MyCharacter);
bool bJumpBlocked = UNomadStatusEffectGameplayHelpers::IsJumpBlocked(MyCharacter);
bool bCustomBlocked = UNomadStatusEffectGameplayHelpers::IsActionBlocked(
    MyCharacter, FGameplayTag::RequestGameplayTag("Status.Block.Interact"));
```

### 3. Movement Status Effects

#### New Status Effect Classes
- `UNomadMovementSpeedStatusEffect` - Base class for all movement modifications
- `UNomadSpeedBoostStatusEffect` - For movement speed increases
- `UNomadSpeedPenaltyStatusEffect` - For movement speed decreases
- `UNomadMovementDisabledStatusEffect` - For complete movement restriction

#### Configuration-Driven Approach
All movement speed values now come from `UNomadInfiniteEffectConfig` data assets:
- `PersistentAttributeModifier` - Defines attribute modifications
- `BlockingTags` - Defines which actions to block
- No hardcoded multipliers or thresholds

### 4. Survival System Integration

#### Old Approach (Mixed)
- Some effects used hardcoded attribute manipulation
- Manual GUID tracking for movement penalties
- Direct stat modifications

#### New Approach (Unified)
- All survival effects use status effect classes
- Config-driven attribute modifiers
- Automatic cleanup and state management

## Migration Steps

### For Existing Code Using Old Methods

1. **Replace hardcoded movement speed sync:**
   ```cpp
   // Old
   UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromStat(Character);
   
   // New
   UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromDefaultAttribute(Character);
   ```

2. **Replace manual movement modifiers:**
   ```cpp
   // Old
   UNomadStatusEffectGameplayHelpers::ApplyMovementSpeedModifierToState(
       MoveComp, State, 0.8f, MyGuid);
   
   // New
   UNomadStatusEffectGameplayHelpers::ApplyMovementSpeedStatusEffect(
       Character, UNomadSpeedPenaltyStatusEffect::StaticClass(), Duration);
   ```

3. **Enhance input blocking:**
   ```cpp
   // Old
   if (UNomadStatusEffectGameplayHelpers::IsSprintBlocked(Character))
   
   // New - More options available
   if (UNomadStatusEffectGameplayHelpers::IsJumpBlocked(Character) ||
       UNomadStatusEffectGameplayHelpers::IsActionBlocked(Character, CustomTag))
   ```

### For New Status Effects

1. **Create effect config assets** using `UNomadInfiniteEffectConfig`
2. **Set PersistentAttributeModifier** for movement speed changes
3. **Set BlockingTags** for input restrictions
4. **Use the new movement status effect classes** as base classes

## Backward Compatibility

- All existing methods are maintained with deprecation warnings
- Gradual migration path available
- New recommended methods provided alongside deprecated ones
- No breaking changes for existing functionality

## Benefits

1. **Data-Driven Design** - All values come from config assets
2. **Flexible Input Blocking** - Support for any action blocking via gameplay tags
3. **Unified API** - Consistent interface for all movement speed modifications
4. **Better Integration** - Seamless integration with existing survival system
5. **Enhanced Debugging** - Helper methods for checking active effects
6. **Maintainability** - No hardcoded values scattered throughout the codebase

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

## Support

For questions about migration or new functionality, refer to:
- Method documentation in `NomadStatusEffectGameplayHelpers.h`
- Status effect class documentation in movement effect headers
- Configuration examples in data asset comments