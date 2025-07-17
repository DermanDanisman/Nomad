# Status Effect Integration - Usage Guide

## Overview

This document explains how to use the newly integrated status effect methods for jump blocking and movement speed synchronization in the Nomad survival system.

## Integration Summary

The following unused methods from `UNomadBaseStatusEffect` have been integrated into the survival mechanics:

- `ApplyJumpBlockTag(ACharacter* Character)` - Blocks jumping for severe conditions
- `RemoveJumpBlockTag(ACharacter* Character)` - Removes jump blocking when conditions improve
- `SyncMovementSpeedModifier(ACharacter* Character, float Multiplier)` - Syncs movement speed from attributes
- `RemoveMovementSpeedModifier(ACharacter* Character)` - Ensures cleanup of movement modifiers

## How It Works

### Jump Blocking Integration

**Automatic Application:**
- Jump blocking is automatically applied when survival effects reach **Severe** or **Extreme** severity levels
- This affects starvation, dehydration, heatstroke, and hypothermia effects
- The blocking is handled via the existing gameplay tag system: `Status.Block.Jump`

**Example Flow:**
1. Player's hunger drops to 0 (starvation)
2. Survival system applies `UNomadStarvationStatusEffect` with Severe severity
3. Effect automatically calls `ApplyJumpBlockTag()` during `OnStatusEffectStarts`
4. Player can no longer jump until hunger improves
5. When hunger improves, effect calls `RemoveJumpBlockTag()` during `OnStatusEffectEnds`

### Movement Speed Synchronization

**Automatic Synchronization:**
- Movement speed is automatically synced whenever status effects are applied or removed
- This ensures the movement component reflects changes from the attribute system
- Works with all status effect types: Infinite, Timed, Instant, and Survival

**Integration Points:**
- `OnStatusEffectStarts` - Syncs speed after applying attribute modifiers
- `OnStatusEffectEnds` - Syncs speed after removing attribute modifiers
- `SetSeverityLevel` - Syncs speed when survival effect severity changes
- `HandleInfiniteTick` - Periodic sync for long-running effects

## Multiplayer Support

The integration leverages the existing ACF status effect replication system:

- **Status Effect Manager**: Handles replication of status effects and blocking tags
- **Attribute System**: Replicates attribute changes that affect movement speed
- **Automatic Sync**: Movement speed changes are automatically reflected on all clients

## Testing the Integration

### Blueprint Testing

Use the provided Blueprint function library for easy testing:

```cpp
// Get the player character
ACharacter* Player = UNomadStatusEffectIntegrationTestLibrary::GetPlayerCharacterForTesting(this);

// Run all integration tests
bool bAllTestsPassed = UNomadStatusEffectIntegrationTestLibrary::RunAllStatusEffectIntegrationTests(Player);

// Or test specific functionality
bool bJumpBlockingWorks = UNomadStatusEffectIntegrationTestLibrary::TestJumpBlockingIntegration(Player);
bool bMovementSyncWorks = UNomadStatusEffectIntegrationTestLibrary::TestMovementSpeedSyncIntegration(Player);
```

### Manual Testing

1. **Test Jump Blocking:**
   ```cpp
   // Simulate severe starvation
   UNomadStatusEffectIntegrationTestLibrary::SimulateSevereConditionsForTesting(PlayerCharacter, true, false, false);
   
   // Try to jump - should be blocked
   // Check status effect UI - should show starvation effect
   
   // Restore normal conditions
   UNomadStatusEffectIntegrationTestLibrary::RestoreNormalConditionsAfterTesting(PlayerCharacter);
   ```

2. **Test Movement Speed:**
   - Apply any status effect with movement speed modifiers
   - Check that movement component speed matches attribute values
   - Remove effect and verify speed returns to normal

### Console Testing

The test functions are Blueprint-callable and can be used in Blueprint graphs or called through Blueprint console commands.

## Configuration

### Jump Blocking Configuration

Jump blocking is automatically applied for Severe/Extreme conditions. To configure which conditions trigger blocking:

1. Modify the severity threshold in `UNomadSurvivalStatusEffect::OnStatusEffectStarts_Implementation`
2. Adjust survival condition thresholds in config data assets

### Movement Speed Configuration

Movement speed sync uses the default movement speed attribute tag: `RPG.Attributes.MovementSpeed`

To use custom attribute tags:
```cpp
// In your status effect config asset:
PersistentAttributeModifier.RPG_Attributes_MovementSpeed = 0.5f;  // 50% speed reduction
```

## Debugging

### Log Categories

The integration uses these log categories for debugging:
- `LogNomadAffliction` - General status effect logs
- `LogNomadSurvival` - Survival system logs
- Both support verbosity levels: Error, Warning, Log, Verbose, VeryVerbose

### Enable Verbose Logging

In your project settings or console:
```
log LogNomadAffliction Verbose
log LogNomadSurvival Verbose
```

### Common Issues

1. **Jump blocking not working:**
   - Check if status effect reached Severe/Extreme severity
   - Verify `UNomadStatusEffectManagerComponent` is present on character
   - Check logs for blocking tag application messages

2. **Movement speed not syncing:**
   - Verify `UARSStatisticsComponent` and `UACFCharacterMovementComponent` are present
   - Check if movement speed attribute exists and has valid values
   - Look for sync method calls in logs

3. **Effects not replicating in multiplayer:**
   - Ensure status effects are applied on the server (authority check)
   - Verify the status effect manager has proper replication setup
   - Check network connection and replication logs

## Files Modified

The integration was implemented with minimal changes to these files:

1. `NomadSurvivalStatusEffect.cpp` - Added jump blocking and movement sync to survival effects
2. `NomadInfiniteStatusEffect.cpp` - Added movement sync to infinite effects  
3. `NomadTimedStatusEffect.cpp` - Added movement sync to timed effects
4. `NomadInstantStatusEffect.cpp` - Added movement sync to instant effects
5. `NomadSurvivalNeedsComponent.cpp` - Added cleanup sync when removing all effects

## Future Enhancements

Potential future improvements:
- Additional blocking tags for other actions (interact, attack, etc.)
- Configurable severity levels for jump blocking
- More granular movement speed modifiers
- Enhanced multiplayer validation
- Integration with animation system for blocked actions