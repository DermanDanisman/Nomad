// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Core/Data/Player/NomadSurvivalNeedsData.h"
#include "StatusEffectSystem/Public/StatusEffects/ACFBaseStatusEffect.h"
#include "NomadSurvivalNeedsComponent.generated.h"

/*
===============================================================================
EDGE CASES & ROBUSTNESS NOTES
===============================================================================
Last Updated: 2025-07-17 by DermanDanisman
Recent Updates: Consolidated status effect system, renamed confusing functions, 
                added proper survival status effects with data-driven configuration

1. Stat Clamping:
    - Stat modification (hunger, thirst, health, body temp) is always clamped.
    - Stats never go below 0, except body temp (can go negative in extreme cases).
    - Stat boundary conditions (at 0, crossing thresholds) are handled for both decay and recovery.
    - Stat changes from other systems (debug, cheats, etc.) are reflected immediately on next tick.

2. Status Effect System (UPDATED - 2025-07-17):
    - NEW: Dual status effect system - generic (old) and survival-specific (new)
    - NEW: Survival effects use UNomadSurvivalStatusEffect with data-driven attribute modifiers
    - OLD: Generic effects continue using TryApplyStatusEffect for compatibility
    - All effects must be set to "Can be Retriggered = TRUE" and "Stacking = FALSE" in asset
    - Status effects handle their own attribute modifiers via config assets (no more manual GUID tracking)

3. Time Jumps (Sleep, Fast Travel, Save/Load):
    - If time advances by >1 minute in a tick, only one OnMinuteTick is processed by default.
    - To ensure correct decay/hazard logic, call OnMinuteTick for each skipped minute or batch process decay.
    - Exposure counters and warning cooldowns are based on in-game time, so long jumps may skip some warnings/events.

4. Component/Config Missing:
    - If StatisticsComponent or SurvivalConfig is missing, almost all simulation logic is skipped or fails safely.
    - Optional: Log errors and failsafe if critical components/config are not found.

5. Designer Error Handling:
    - DataAsset fields are clamped and sanity-checked.
    - If a designer sets an invalid (e.g., negative) threshold or cooldown, gameplay may be unpredictable.
    - Recommend DataAsset validation at startup.

6. Movement Slow/Recover (UPDATED - 2025-07-17):
    - OLD: Manual attribute manipulation with hard-coded GUIDs (REMOVED)
    - NEW: Status effects handle movement penalties via config-driven attribute modifiers
    - Events still fire for UI/analytics but actual penalties are applied by status effects

7. Multiple Hazards:
    - Different hazard effects (starvation, dehydration, heatstroke, hypothermia) are managed independently and can overlap.
    - Same effect will never stack; each is managed separately by tag/class.
    - NEW: Survival effects are removed/reapplied each tick for clean state transitions

8. Save/Load:
    - If you support save/load, ensure all survival state (stats, counters, cooldowns, flags) is serialized and restored.
    - NEW: Status effects persist automatically via ACF status effect manager

9. Per-Player Temperature System:
    - Each player samples temperature at their specific world location via BP_GetTemperatureAtPlayerLocation().
    - Global UDS temperature is NOT used directly; component gets player-specific temperature each tick.
    - Temperature is replicated to clients for UI updates; server calculates survival effects.
    - If BP_GetTemperatureAtPlayerLocation() returns invalid values, component will clamp to safe range (-100°C to 100°C).
    - Players in different locations can experience different temperatures simultaneously.
    - Temperature modifiers (hunger/thirst decay) use normalized temperature [0..1] based on config min/max ranges.
    - Body temperature simulation has two modes: safe zone (trends to normal) and extreme zone (influenced by ambient).
    - Heatstroke/hypothermia require sustained exposure (configurable duration) rather than instant triggers.
    - Temperature unit (Celsius/Fahrenheit) affects input interpretation but internal calculations use consistent ranges.
    - Network replication ensures client UI shows accurate temperature while server maintains authority for gameplay effects.

10. Escalating Warning System (NEW - 2025-07-04):
    - Warning frequencies accelerate over time: 1st warning (normal), 3rd+ (2x faster), 6th+ (4x faster).
    - Warning counters reset to 0 when player recovers from condition (hunger/thirst above threshold).
    - Each warning type (starvation, dehydration, heatstroke, hypothermia) has independent escalation counters.
    - Warning cooldown timers use in-game time, so fast travel/time skips may cause warnings to appear immediately.
    - ShouldFireEscalatingWarning() uses string comparison for warning types - ensure consistent naming.

11. Dual Notification System (NEW - 2025-07-04):
    - Specific delegates (OnStarvationWarning, etc.) fire for gameplay systems that need raw stat values.
    - Unified OnSurvivalNotification delegate fires for UI systems that need formatted text/colors.
    - Both systems fire simultaneously - no duplication, but UI gets rich notification data.
    - Notification text includes emojis - ensure UI systems support Unicode rendering.
    - BroadcastSurvivalNotification() includes debug on-screen messages in development builds only.

12. Warning State Management (NEW - 2025-07-04):
    - bStarvationWarningGiven flags track if initial warnings were issued (different from periodic warnings).
    - Periodic warnings continue firing based on cooldowns while condition persists.
    - EvaluateWeatherHazards() only manages warning state flags, actual broadcasts happen in MaybeFireXXXWarning().
    - Warning state flags reset when player exits warning condition, ensuring immediate warning on re-entry.

13. Memory & Performance (NEW - 2025-07-04):
    - FCachedStatValues struct minimizes StatisticsComponent calls per tick (single lookup vs multiple).
    - ShouldFireEscalatingWarning() uses pointer arithmetic to avoid switch statements for performance.
    - String comparisons in warning system are not performance-critical (once per minute max per player).
    - OnSurvivalNotification delegate should not be overused - UI systems should cache notifications locally.

14. Multiplayer Considerations (NEW - 2025-07-04):
    - Only server runs survival simulation logic (HasAuthority() check in OnMinuteTick).
    - Clients receive replicated state for UI but cannot modify survival values directly.
    - OnSurvivalNotification fires on server only - ensure UI systems handle this correctly.
    - Warning counts and cooldown timers are server-only and not replicated (reduces network traffic).
    - LastExternalTemperature replication allows clients to show current temperature without server round-trip.

15. Function Consolidation (NEW - 2025-07-17):
    - RENAMED: EvaluateAndUpdateSurvivalState -> UpdateSurvivalUIState (clearer purpose)
    - REMOVED: TryApplyStatusEffect (replaced with ApplyStatusEffect + ApplyGenericStatusEffect)
    - ADDED: EvaluateAndApplySurvivalEffects (new main entry point for survival status effects)
    - CLEAR: Each function has single responsibility - simulation, events, or status effects

===============================================================================
*/

// Forward declarations
class UNomadStatusEffectManagerComponent;
class UNomadSurvivalHazardConfig;
class UNomadSurvivalStatusEffect;

// ----------------------------------------------------------------
// Survival Severity Enum
// ----------------------------------------------------------------
/**
 * ESurvivalSeverity
 * -----------------
 * Enum for different severity levels of survival status effects.
 * Used to categorize the intensity of survival conditions.
 */
UENUM(BlueprintType)
enum class ESurvivalSeverity : uint8
{
    None        UMETA(DisplayName = "None"),
    Mild        UMETA(DisplayName = "Mild"),        // Early warning stage
    Heavy       UMETA(DisplayName = "Heavy"),       // Moderate penalty stage  
    Severe      UMETA(DisplayName = "Severe"),      // Critical stage with major penalties
    Extreme     UMETA(DisplayName = "Extreme")      // Life-threatening stage
};

// ----------------------------------------------------------------
// Temperature unit enum for UDS external readings
// ----------------------------------------------------------------
/**
 * ETemperatureUnit
 * ----------------
 * Enum for choosing temperature units for external readings.
 * Used for interpreting values from Ultra Dynamic Sky/weather providers.
 */
UENUM(BlueprintType)
enum class ETemperatureUnit : uint8
{
    Celsius     UMETA(DisplayName = "Celsius"),    // Units in degrees Celsius
    Fahrenheit  UMETA(DisplayName = "Fahrenheit")  // Units in degrees Fahrenheit
};

// ----------------------------------------------------------------
// Unified Survival State Enum
// ----------------------------------------------------------------
/**
 * ESurvivalState
 * --------------
 * Enum representing the player's overall survival status.
 * Used for generic state transitions and UI.
 */
UENUM(BlueprintType)
enum class ESurvivalState : uint8
{
    Normal          UMETA(DisplayName="Normal"),
    Hungry          UMETA(DisplayName="Hungry"),
    Starving        UMETA(DisplayName="Starving"),
    Thirsty         UMETA(DisplayName="Thirsty"),
    Dehydrated      UMETA(DisplayName="Dehydrated"),
    Heatstroke      UMETA(DisplayName="Heatstroke"),
    Hypothermic     UMETA(DisplayName="Hypothermic")
};

class UARSStatisticsComponent;

// ----------------------------------------------------------------
// Blueprint Event Delegates
// ----------------------------------------------------------------
/**
 * Delegates broadcast important simulation events for UI/Blueprints.
 * All delegates are multicast (can bind multiple listeners).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDecaysComputed, float, CalculatedHungerDecay, float, CalculatedThirstDecay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStarvationWarning, float, CurrentHunger);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStarvationStarted, float, CurrentHunger);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStarvationEnded, float, CurrentHunger);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDehydrationWarning, float, CurrentThirst);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDehydrationStarted, float, CurrentThirst);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDehydrationEnded, float, CurrentThirst);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovementSlowed, FName, SlowCause, float, StatValueAtSlow);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovementRecovered, float, StatValueAtRecover);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeatstrokeWarning, float, BodyTemperature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeatstrokeStarted, float, BodyTemperature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeatstrokeEnded, float, BodyTemperature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHypothermiaWarning, float, BodyTemperature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHypothermiaStarted, float, BodyTemperature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHypothermiaEnded, float, BodyTemperature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSurvivalStateChanged, ESurvivalState, OldState, ESurvivalState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSurvivalNotification, FString, NotificationText, FLinearColor, NotificationColor, float, DisplayDuration);

/**
 * UNomadSurvivalNeedsComponent
 * ----------------------------
 * Main component for managing player survival needs (hunger, thirst, body temperature, etc.).
 * All tuning variables are now sourced from the DataAsset (SurvivalConfig).
 * All cooldowns and warning thresholds are now configurable via DataAsset.
 * All time-based logic uses UDS-provided in-game time for synchrony.
 * 
 * NEW (2025-07-17): Dual status effect system with data-driven survival effects
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOMADDEV_API UNomadSurvivalNeedsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // ======== Initialization ========

    /** Constructor. Initializes default variables. */
    UNomadSurvivalNeedsComponent();

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    /** 
     * Current overall survival state (replicated to clients for UI).
     * Automatically calculated based on most severe condition affecting the player.
     * Priority: Heatstroke > Hypothermic > Starving > Dehydrated > Hungry > Thirsty > Normal
     */
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Survival|Status")
    ESurvivalState CurrentSurvivalState = ESurvivalState::Normal;
    
    /** 
     * Most recent external temperature received at player's location (replicated for client UI).
     * Updated every OnMinuteTick() based on player's specific world position.
     * Units determined by TemperatureUnit setting (Celsius/Fahrenheit).
     */
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Survival|Temperature")
    float LastExternalTemperature = 0.f;

    /** 
     * Cached normalized temperature value [0..1] for UI display (replicated).
     * 0 = coldest possible temperature, 1 = hottest possible temperature.
     * Automatically calculated from LastExternalTemperature using config ranges.
     */
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Survival|Temperature")
    float LastTemperatureNormalized = 0.f;

    /** 
     * C++ wrapper function that calls the Blueprint implementable event.
     * Allows Blueprint classes to define custom location-based temperature logic.
     * Used for per-player temperature sampling instead of global weather values.
     */
    UFUNCTION(BlueprintCallable, Category = "Survival|Temperature")
    float GetTemperatureAtPlayerLocation() const;
    
    /** 
     * Blueprint implementable event for location-based temperature sampling.
     * Override in Blueprint to implement custom logic for getting temperature at player's position.
     * Should query UDS/weather system using player's world location and return temperature value.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Survival|Temperature")
    float BP_GetTemperatureAtPlayerLocation() const;
    
    // ======== Main Simulation Tick ========

    /**
     * Main tick function for survival simulation.
     * Called once per in-game minute by the time/weather system.
     * @param TimeOfDay Current in-game time (minutes since midnight, from UDS)
     */
    UFUNCTION(BlueprintCallable, Category = "Survival|Tick")
    void OnMinuteTick(float TimeOfDay);

    // ======== Blueprint Utility Getters (for UI) ========

    /** Returns current hunger as a normalized percent [0..1] for progress bars, etc. */
    UFUNCTION(BlueprintPure, Category = "Survival|Status")
    float GetHungerPercent() const;

    /** Returns current thirst as a normalized percent [0..1] for progress bars, etc. */
    UFUNCTION(BlueprintPure, Category = "Survival|Status")
    float GetThirstPercent() const;

    /** Changes the temperature unit (Celsius/Fahrenheit) at runtime. */
    UFUNCTION(BlueprintCallable, Category = "Survival|Temperature")
    void SetTemperatureUnit(const ETemperatureUnit InUnit) { TemperatureUnit = InUnit; }

    /** 
     * Returns the last normalized environmental temperature [0..1].
     * 0 = coldest possible, 1 = hottest possible (for UI bars, hazard logic, etc).
     */
    UFUNCTION(BlueprintPure, Category="Survival|Temperature")
    float GetTemperatureNormalized(float InExternalTemperature) const;
    
    /** Returns normalized temperature [0..1] for advanced curve input (NOT for UI bars). */
    float GetNormalizedTemperatureForCurve(float InExternalTemperature) const;

    /** Returns normalized activity [0..1] for advanced curve input. */
    float GetNormalizedActivity() const;

    /**
     * Returns the last external temperature from UDS/weather system.
     * Since UDS can provide temperature in the unit you want, this just returns the cached value.
     */
    UFUNCTION(BlueprintPure, Category="Survival|Temperature")
    float GetExternalTemperature() const
    {
        return LastExternalTemperature;
    }

    /**
     * Returns true if the external temperature is above freezing (0°C equivalent).
     * Useful for UI color changes or warnings.
     */
    UFUNCTION(BlueprintPure, Category="Survival|Temperature")
    bool IsAboveFreezing() const
    {
        const float FreezingPoint = (TemperatureUnit == ETemperatureUnit::Celsius) ? 0.f : 32.f;
        return LastExternalTemperature >= FreezingPoint;
    }

    // ======== Blueprint Event Dispatchers (UI/gameplay hooks) ========

    /** Broadcasts when current hunger/thirst decay is computed (for UI/analytics). */
    // --- Delegates (BlueprintAssignable for BP binding) ---
    
    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnDecaysComputed OnDecaysComputed;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnStarvationWarning OnStarvationWarning;
    
    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnStarvationStarted OnStarvationStarted;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnStarvationEnded OnStarvationEnded;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnDehydrationWarning OnDehydrationWarning;
    
    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnDehydrationStarted OnDehydrationStarted;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnDehydrationEnded OnDehydrationEnded;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnMovementSlowed OnMovementSlowed;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnMovementRecovered OnMovementRecovered;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnHeatstrokeWarning OnHeatstrokeWarning;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnHeatstrokeStarted OnHeatstrokeStarted;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnHeatstrokeEnded OnHeatstrokeEnded;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnHypothermiaWarning OnHypothermiaWarning;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnHypothermiaStarted OnHypothermiaStarted;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnHypothermiaEnded OnHypothermiaEnded;

    UPROPERTY(BlueprintAssignable, Category="Survival|Events")
    FOnSurvivalStateChanged OnSurvivalStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Survival|Events")
    FOnSurvivalNotification OnSurvivalNotification;

protected:
    // ======== Unreal Lifecycle ========

    /** Called when the game starts or when spawned. Caches references, initializes simulation. */
    virtual void BeginPlay() override;

    // Add helper to get config asset as before
    FORCEINLINE UNomadSurvivalNeedsData* GetConfig() const { return SurvivalConfig; }

    /**
     * Selects which temperature unit (Celsius or Fahrenheit) is used for input from weather system.
     * Can be changed at runtime for localization.
     */
    UPROPERTY(EditAnywhere, Category = "Survival|Temperature", meta = (ToolTip = "Unit for external UDS temperature"))
    ETemperatureUnit TemperatureUnit = ETemperatureUnit::Celsius;

    // ======== Blueprint Implementable Events ========
    /**
     * Override in Blueprint to provide custom logic for heat protection gear (e.g. clothes, buffs).
     * Return true if character is protected from heat-based hazards.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Survival|Gear")
    bool HasHeatProtectionGear() const;

    /**
     * Override in Blueprint to provide custom logic for cold protection gear (e.g. insulation, buffs).
     * Return true if character is protected from cold-based hazards.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Survival|Gear")
    bool HasColdProtectionGear() const;

private:
    // ======== Constants ========
    
    static constexpr float MINUTES_PER_DAY = 24.f * 60.f;
    static constexpr float TEMPERATURE_VALIDATION_MIN = -100.f;
    static constexpr float TEMPERATURE_VALIDATION_MAX = 100.f;

    // ======== Internal State ========

    /** Cached reference to player's ARS statistics component (for hunger/thirst/health/body temp). */
    UPROPERTY()
    TObjectPtr<UARSStatisticsComponent> StatisticsComponent = nullptr;

    /** Cached reference to player's status effect manager component. */
    UPROPERTY()
    TObjectPtr<UNomadStatusEffectManagerComponent> StatusEffectManagerComponent = nullptr;

    /** 
     * Data Asset holding all tunable survival parameters.
     * Designers assign this in the editor (see UNomadSurvivalNeedsData).
     */
    UPROPERTY(EditDefaultsOnly, Category="Survival|Data")
    TObjectPtr<UNomadSurvivalNeedsData> SurvivalConfig;
    
    /** Base hunger lost per minute at rest, after config is loaded. */
    float BaseHungerPerMinute = 0.f;

    /** Base thirst lost per minute at rest, after config is loaded. */
    float BaseThirstPerMinute = 0.f;

    /** True if player is currently starving (hunger stat at/below 0). */
    bool bIsStarving = false;

    /** True if player is currently dehydrated (thirst stat at/below 0). */
    bool bIsDehydrated = false;

    /** True if player's movement is currently slowed (due to hunger/thirst). */
    bool bMovementSlowed = false;

    /** True if player is currently suffering heatstroke (body temp hazard). */
    bool bInHeatstroke = false;

    /** True if player is currently suffering hypothermia (body temp hazard). */
    bool bInHypothermia = false;

    /** Counts consecutive minutes spent in heat hazard zone (for triggering heatstroke). */
    int32 HeatExposureCounter = 0;

    /** Counts consecutive minutes spent in cold hazard zone (for triggering hypothermia). */
    int32 ColdExposureCounter = 0;

    // ======== Cooldown Timers ========
    
    /** Last in-game time (minutes since midnight) a starvation warning was fired. */
    float LastStarvationWarningTime;

    /** Last in-game time (minutes since midnight) a dehydration warning was fired. */
    float LastDehydrationWarningTime;

    /** Last in-game time (minutes since midnight) a heatstroke warning was fired. */
    float LastHeatstrokeWarningTime;

    /** Last in-game time (minutes since midnight) a hypothermia warning was fired. */
    float LastHypothermiaWarningTime;

    // ======== Warning Event Flags ========

    /** True if starvation warning has been issued (reset on recovery). */
    bool bStarvationWarningGiven = false;

    /** True if dehydration warning has been issued (reset on recovery). */
    bool bDehydrationWarningGiven = false;

    /** True if heatstroke warning has been issued (reset on recovery). */
    bool bHeatstrokeWarningGiven = false;

    /** True if hypothermia warning has been issued (reset on recovery). */
    bool bHypothermiaWarningGiven = false;

    UPROPERTY()
    int32 StarvationWarningCount = 0;

    UPROPERTY()
    int32 DehydrationWarningCount = 0;

    UPROPERTY()
    int32 HeatstrokeWarningCount = 0;

    UPROPERTY()
    int32 HypothermiaWarningCount = 0;

    // ======== Cached Values Struct ========
    
    /** Structure for caching frequently accessed stat values */
    struct FCachedStatValues
    {
        float Hunger = 0.f;
        float Thirst = 0.f;
        float BodyTemp = 0.f;
        float Endurance = 0.f;
        bool bValid = false;
        
        FCachedStatValues() = default;
    };

    // ======== Internal Helpers ========

    /** Returns true if stat value is valid (non-negative). */
    FORCEINLINE bool IsValidStatValue(const float Value) const { return Value >= 0.f; }

    /** Updates and returns cached stat values to avoid redundant component calls */
    FCachedStatValues GetCachedStatValues() const;

    // ======== Core Simulation Helpers ========
    
    /**
     * Normalizes the given temperature to [0,1] using climate config.
     * @param InRawTemperature   The temperature reading from weather/UDS.
     * @param bIsWarmBar       Whether this is for a warm temperature bar (above freezing).
     * @return Value in [0,1] for use in all modifiers/hazards.
     */
    float ComputeNormalizedTemperature(float InRawTemperature, bool bIsWarmBar) const;
    
    // ======== Advanced Curve-Driven Modifiers ========
    
    /**
     * Calculates extra hunger decay modifier due to cold temperature.
     * @param NormalizedTempForCurve  Pre-calculated normalized temperature for curve input.
     * @return Modifier value to add to base decay.
     */
    float ComputeColdHungerModifier(float NormalizedTempForCurve) const;

    /**
     * Calculates extra thirst decay modifier due to hot temperature.
     * @param NormalizedTempForCurve  Pre-calculated normalized temperature for curve input.
     * @return Modifier value to add to base decay.
     */
    float ComputeHotThirstModifier(float NormalizedTempForCurve) const;

    /**
     * Temperature-driven hunger modifier using advanced designer curve.
     * @param NormalizedActivity  Pre-calculated normalized activity value.
     * @return Modifier value to add to base decay.
     */
    float ComputeHungerActivityModifier(float NormalizedActivity) const;

    /**
     * Activity-driven thirst modifier using advanced designer curve.
     * @param NormalizedActivity  Pre-calculated normalized activity value.
     * @return Modifier value to add to base decay.
     */
    float ComputeThirstActivityModifier(float NormalizedActivity) const;

    /**
     * Applies calculated hunger and thirst decay to ARS stats.
     * @param HungerDecay   Value to subtract from hunger stat.
     * @param ThirstDecay   Value to subtract from thirst stat.
     */
    void ApplyDecayToStats(float HungerDecay, float ThirstDecay) const;
    
    /**
     * Gets appropriate notification text for temperature effects based on tag and config
     * @param EffectTag The gameplay tag of the temperature effect
     * @param Config The survival needs configuration
     * @return Formatted notification text with multiplier values
     */
    FString GetTemperatureNotificationText(const FGameplayTag& EffectTag, const UNomadSurvivalNeedsData* Config) const;
    
    /**
     * Gets the appropriate color for UI notifications based on severity
     * @param Severity The severity level of the effect
     * @return Color for UI display (Yellow for mild, Orange for heavy, Red for severe/extreme)
     */
    FLinearColor GetSeverityColor(ESurvivalSeverity Severity) const;

    // ======== Event System (State Transitions & Warnings) ========

    /**
     * Fires starvation/dehydration events on stat threshold crossings using cached values.
     * Handles state change events (OnStarvationStarted/Ended, OnDehydrationStarted/Ended).
     * Does NOT apply status effects - that's handled by EvaluateAndApplySurvivalEffects.
     * @param CachedValues  Pre-calculated stat values.
     */
    void EvaluateSurvivalStateTransitions(const FCachedStatValues& CachedValues);

    /**
     * Checks for and fires hazard warnings (heatstroke, hypothermia) using cached values.
     * Manages internal warning state flags for temperature hazards.
     * Actual warning broadcasts happen in MaybeFireXXXWarning() functions.
     * @param CachedValues  Pre-calculated stat values.
     */
    void EvaluateWeatherHazards(const FCachedStatValues& CachedValues);

    /**
     * Main body temperature simulation using cached values.
     * Handles physics-based body temperature changes and exposure counters.
     * Fires temperature hazard start/end events (OnHeatstrokeStarted/Ended, OnHypothermiaStarted/Ended).
     * @param AmbientTempCelsius  Current ambient temperature (converted to Celsius if needed).
     * @param CachedValues  Pre-calculated stat values.
     */
    void UpdateBodyTemperature(float AmbientTempCelsius, const FCachedStatValues& CachedValues);
    
    /**
     * Evaluates and updates player's overall survival state for UI using cached values.
     * RENAMED FROM: EvaluateAndUpdateSurvivalState (clearer naming - this is UI-only).
     * Updates CurrentSurvivalState enum for client UI display.
     * @param CachedValues  Pre-calculated stat values.
     */
    void UpdateSurvivalUIState(const FCachedStatValues& CachedValues);

    // ======== Status Effect System (NEW - 2025-07-17) ========

    /**
     * NEW: Main entry point for survival status effect evaluation.
     * Evaluates current survival conditions and applies/removes appropriate status effects.
     * Replaces the old manual attribute manipulation system with data-driven status effects.
     * Called from OnMinuteTick after stat updates.
     * @param CachedValues  Pre-calculated stat values.
     */
    void EvaluateAndApplySurvivalEffects(const FCachedStatValues& CachedValues);

    /**
     * NEW: Evaluates hunger levels and applies appropriate starvation effects.
     * Handles both mild hunger warnings and severe starvation with DoT.
     * Uses data-driven UNomadSurvivalStatusEffect classes with config-based attribute modifiers.
     * @param HungerLevel Current hunger stat value.
     */
    void EvaluateHungerEffects(float HungerLevel);

    /**
     * NEW: Evaluates thirst levels and applies appropriate dehydration effects.
     * Handles both mild thirst warnings and severe dehydration with DoT.
     * Uses data-driven UNomadSurvivalStatusEffect classes with config-based attribute modifiers.
     * @param ThirstLevel Current thirst stat value.
     */
    void EvaluateThirstEffects(float ThirstLevel);

    /**
     * NEW: Evaluates body temperature and applies appropriate temperature effects.
     * Handles both heatstroke and hypothermia with multiple severity levels.
     * Uses data-driven UNomadSurvivalStatusEffect classes with config-based attribute modifiers.
     * @param BodyTemp Current body temperature stat value.
     */
    void EvaluateTemperatureEffects(float BodyTemp);

    /**
     * NEW: Applies a survival status effect using the ACF status effect system.
     * Automatically sets appropriate severity and DoT values based on config.
     * Uses UNomadSurvivalStatusEffect with data-driven attribute modifiers.
     * @param EffectClass Status effect class to apply.
     * @param Severity Severity level for visual/audio effects.
     * @param DoTPercent Damage over time percentage (0.0 for no DoT).
     */
    void ApplyStatusEffect(TSubclassOf<UNomadSurvivalStatusEffect> EffectClass, ESurvivalSeverity Severity, float DoTPercent = 0.0f);

    /**
     * NEW: Removes all active survival status effects.
     * Called when conditions improve or for cleanup during state transitions.
     */
    void RemoveAllSurvivalEffects();

    // ======== Warning System ========

    /**
     * Cooldown-based warning helpers using cached values.
     */
    void MaybeFireStarvationWarning(float CurrentInGameTime, float CurrentHunger);
    void MaybeFireDehydrationWarning(float CurrentInGameTime, float CurrentThirst);
    void MaybeFireHeatstrokeWarning(float CurrentInGameTime, float InBodyTemperature);
    void MaybeFireHypothermiaWarning(float CurrentInGameTime, float InBodyTemperature);

    /** Helper function for escalating warnings */
    UFUNCTION()
    bool ShouldFireEscalatingWarning(const FString& WarningType, float CurrentTime, float BaseCooldown, int32& WarningCount);

    /** Helper function to broadcast notifications */
    UFUNCTION()
    void BroadcastSurvivalNotification(const FString& NotificationText, const FLinearColor& Color, float Duration = 3.0f) const;

    // ======== State Check Helpers ========

    /** Helper functions for state checks using cached values. */
    bool IsStarving(float CachedHunger) const;
    bool IsHungry(float CachedHunger) const;
    bool IsDehydrated(float CachedThirst) const;
    bool IsThirsty(float CachedThirst) const;
    bool IsHeatstroke(float CachedBodyTemp) const;
    bool IsHypothermic(float CachedBodyTemp) const;
};