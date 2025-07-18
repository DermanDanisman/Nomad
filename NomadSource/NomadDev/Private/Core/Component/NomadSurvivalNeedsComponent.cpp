// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Component/NomadSurvivalNeedsComponent.h"

#include "ACFCCTypes.h"
#include "ARSStatisticsComponent.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "Core/StatusEffect/SurvivalHazard/NomadSurvivalStatusEffect.h"
#include "Core/Data/StatusEffect/NomadInfiniteEffectConfig.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UNomadSurvivalNeedsComponent::UNomadSurvivalNeedsComponent()
{
    // Disable regular ticking; this component is stepped by external manager (e.g. UDS clock)
    // This prevents unnecessary CPU usage and ensures survival ticks are synchronized with game time
    PrimaryComponentTick.bCanEverTick = false;
    
    // Enable replication for multiplayer support - essential for client UI updates
    // Clients need to see survival state changes without constant server requests
    SetIsReplicatedByDefault(true);

    // Initialize warning timers to -1 (never warned)
    // Negative values indicate no warning has been fired yet, ensuring immediate first warning
    LastStarvationWarningTime = -1.f;
    LastDehydrationWarningTime = -1.f;
    LastHeatstrokeWarningTime = -1.f;
    LastHypothermiaWarningTime = -1.f;
}

void UNomadSurvivalNeedsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    // Replicate essential survival data to clients for UI display
    // Each player only receives their own component's data to reduce network traffic
    
    // Current temperature at player location - used for UI temperature displays
    DOREPLIFETIME(UNomadSurvivalNeedsComponent, LastExternalTemperature);
    
    // Overall survival status - used for UI state indicators and screen effects
    DOREPLIFETIME(UNomadSurvivalNeedsComponent, CurrentSurvivalState);
    
    // Normalized temp for UI bars - pre-calculated to reduce client computation
    DOREPLIFETIME(UNomadSurvivalNeedsComponent, LastTemperatureNormalized);
}

void UNomadSurvivalNeedsComponent::BeginPlay()
{
    // Log component initialization for debugging survival system startup
    SURVIVAL_LOG_ENTER("BeginPlay");
    
    Super::BeginPlay();

    // Comprehensive validation with better error handling
    // Early exit if configuration is missing to prevent crashes
    if (!SurvivalConfig)
    {
        UE_LOG_SURVIVAL(Error, TEXT("SurvivalConfig is null on %s! Survival system will not function."), 
                       *GetOwner()->GetName());
        return; // Critical failure - cannot continue without config
    }
    
    // Cache component references to avoid expensive FindComponentByClass calls during ticks
    // These components are required for survival system functionality
    StatisticsComponent = GetOwner()->FindComponentByClass<UARSStatisticsComponent>();
    StatusEffectManagerComponent = GetOwner()->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    
    // StatisticsComponent is critical - survival cannot function without it
    if (!StatisticsComponent)
    {
        UE_LOG_SURVIVAL(Error, TEXT("ARSStatisticsComponent missing on %s! Survival system will not function."), 
                       *GetOwner()->GetName());
        return; // Critical failure - cannot modify stats without this component
    }
    
    // StatusEffectManagerComponent is optional but recommended for gameplay effects
    if (!StatusEffectManagerComponent)
    {
        UE_LOG_SURVIVAL(Warning, TEXT("NomadStatusEffectManagerComponent missing on %s - status effects will not work"), 
                       *GetOwner()->GetName());
        // Continue execution - survival works without status effects, just less gameplay depth
    }
    
    // Validate config data to prevent divide-by-zero and other math errors
    // Negative or zero values would break the decay calculation system
    if (GetConfig()->GetDailyHungerLoss() <= 0.f || GetConfig()->GetDailyThirstLoss() <= 0.f)
    {
        UE_LOG_SURVIVAL(Error, TEXT("Invalid daily loss values in SurvivalConfig on %s"), 
                       *GetOwner()->GetName());
        return; // Critical failure - cannot calculate decay rates
    }
    
    // Calculate per-minute base decay from 24-hour totals
    // This converts designer-friendly "daily loss" values into simulation-ready per-minute rates
    BaseHungerPerMinute = GetConfig()->GetDailyHungerLoss() / MINUTES_PER_DAY;
    BaseThirstPerMinute = GetConfig()->GetDailyThirstLoss() / MINUTES_PER_DAY;
    
    // Log successful initialization with calculated values for debugging
    UE_LOG_SURVIVAL(Log, TEXT("Survival system initialized on %s. Hunger: %.4f/min, Thirst: %.4f/min"), 
                   *GetOwner()->GetName(), BaseHungerPerMinute, BaseThirstPerMinute);
    
    SURVIVAL_LOG_EXIT("BeginPlay");
}

float UNomadSurvivalNeedsComponent::GetHungerPercent() const
{
    // Early exit if required components are missing
    if (!StatisticsComponent || !GetConfig()) return 0.f;
    
    // Get current and maximum hunger values from statistics system
    const float Current = StatisticsComponent->GetCurrentValueForStatitstic(GetConfig()->GetHungerStatTag());
    const float Max = StatisticsComponent->GetMaxValueForStatitstic(GetConfig()->GetHungerStatTag());
    
    // Return normalized percentage [0..1] for UI progress bars
    // Validate values to prevent NaN/infinity from bad data
    return (IsValidStatValue(Current) && Max > 0.f) ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f;
}

float UNomadSurvivalNeedsComponent::GetThirstPercent() const
{
    // Early exit if required components are missing
    if (!StatisticsComponent || !GetConfig()) return 0.f;
    
    // Get current and maximum thirst values from statistics system
    const float Current = StatisticsComponent->GetCurrentValueForStatitstic(GetConfig()->GetThirstStatTag());
    const float Max = StatisticsComponent->GetMaxValueForStatitstic(GetConfig()->GetThirstStatTag());
    
    // Return normalized percentage [0..1] for UI progress bars
    // Validate values to prevent NaN/infinity from bad data
    return (IsValidStatValue(Current) && Max > 0.f) ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f;
}

float UNomadSurvivalNeedsComponent::GetTemperatureNormalized(const float InExternalTemperature) const
{
    // Delegate to internal computation function with UI-appropriate warm/cold bar logic
    // IsAboveFreezing() determines if this should be treated as warm bar (true) or cold bar (false)
    return ComputeNormalizedTemperature(InExternalTemperature, IsAboveFreezing());
}

float UNomadSurvivalNeedsComponent::GetNormalizedTemperatureForCurve(const float InExternalTemperature) const
{
    // Early exit if config is missing
    if (!GetConfig()) return 0.f;
    
    // Get temperature range from config for normalization
    const float MinT = GetConfig()->GetMinExternalTempCelsius();
    const float MaxT = GetConfig()->GetMaxExternalTempCelsius();
    
    // Normalize temperature to [0..1] range for curve input
    // This is different from UI normalization - it's purely mathematical for curve lookups
    return FMath::Clamp((InExternalTemperature - MinT) / (MaxT - MinT), 0.f, 1.f);
}

float UNomadSurvivalNeedsComponent::GetNormalizedActivity() const
{
    // Early exit if config is missing
    if (!GetConfig()) return 0.f;
    
    // Try to get character owner to access velocity information
    if (const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
    {
        // Calculate movement speed from velocity magnitude
        const float Speed = CharacterOwner->GetVelocity().Size();
        
        // Get threshold values from config for activity level determination
        const float Walk = GetConfig()->GetWalkingSpeedThreshold();
        const float Sprint = GetConfig()->GetSprintingSpeedThreshold();
        
        // Determine activity level based on movement speed
        if (Speed <= Walk) return 0.f; // standing/walking - no extra activity modifier
        if (Speed >= Sprint) return 1.f; // sprinting - maximum activity modifier
        
        // Normalize between walking and sprinting for partial activity modifier
        // This creates smooth transitions between activity levels
        return FMath::Clamp((Speed - Walk) / (Sprint - Walk), 0.f, 1.f);
    }
    
    // Default to no activity if character owner is not available
    return 0.f;
}

float UNomadSurvivalNeedsComponent::GetTemperatureAtPlayerLocation() const
{
    // Delegate to Blueprint implementation for location-based temperature sampling
    // This allows designers to customize temperature logic per-player based on world position
    // Blueprint can query UDS weather system, check for indoor/outdoor, apply local modifiers, etc.
    return BP_GetTemperatureAtPlayerLocation();
}

void UNomadSurvivalNeedsComponent::OnMinuteTick(const float TimeOfDay)
{
    // Server authority: Only run survival logic on server to prevent cheating
    // Clients receive updated values via replication for UI display
    if (!GetOwner()->HasAuthority())
    {
        return; // Early exit for clients - they don't run simulation logic
    }
    
    // Validate required components before proceeding with simulation
    if (!StatisticsComponent || !GetConfig()) 
    {
        UE_LOG_SURVIVAL(Warning, TEXT("OnMinuteTick called but required components/config missing"));
        return; // Cannot proceed without these critical components
    }
        
    // Cache all stat values once per tick to avoid redundant component lookups
    // This optimization reduces expensive StatisticsComponent calls from ~10+ to 1 per tick
    const FCachedStatValues CachedValues = GetCachedStatValues();
    if (!CachedValues.bValid)
    {
        UE_LOG_SURVIVAL_STATS(Warning, TEXT("Failed to cache stat values in OnMinuteTick"));
        return; // Cannot proceed with invalid stat data
    }
        
    // Verbose logging for development debugging - shows all cached values
    UE_LOG_SURVIVAL_STATS(VeryVerbose, TEXT("Cached Stats - H:%.2f T:%.2f BT:%.2f E:%.2f"), 
                         CachedValues.Hunger, CachedValues.Thirst, CachedValues.BodyTemp, CachedValues.Endurance);
    
    // Get player-specific temperature instead of global weather temperature
    // This allows different players to experience different temperatures based on location
    // Example: Player A in desert = 45°C, Player B in cave = 15°C
    const float PlayerLocationTemperature = GetTemperatureAtPlayerLocation();
    
    // Cache temperature values for replication to client UI
    // Pre-calculating these reduces client-side computation load
    LastExternalTemperature = PlayerLocationTemperature;
    LastTemperatureNormalized = GetTemperatureNormalized(PlayerLocationTemperature);
    
    // Pre-calculate normalized values for curve lookups to avoid redundant calculations
    const float NormalizedTempForCurve = GetNormalizedTemperatureForCurve(PlayerLocationTemperature);
    const float NormalizedActivity = GetNormalizedActivity();
    
    // Calculate environmental and activity modifiers using designer curves
    // These modifiers add/subtract from base decay rates based on conditions
    const float HungerTemperatureMod = ComputeColdHungerModifier(NormalizedTempForCurve);
    const float ThirstTemperatureMod = ComputeHotThirstModifier(NormalizedTempForCurve);
    const float HungerActivityMod = ComputeHungerActivityModifier(NormalizedActivity);
    const float ThirstActivityMod = ComputeThirstActivityModifier(NormalizedActivity);
    
    // Apply endurance-based reduction to base decay rates
    // Higher endurance = slower hunger/thirst decay (survival skill simulation)
    const float EffectiveHungerBase = FMath::Max(0.f, 
        BaseHungerPerMinute * (1.f - CachedValues.Endurance * GetConfig()->GetEnduranceDecayPerPoint()));
    const float EffectiveThirstBase = FMath::Max(0.f, 
        BaseThirstPerMinute * (1.f - CachedValues.Endurance * GetConfig()->GetEnduranceDecayPerPoint()));
    
    float TemperatureHungerMultiplier, TemperatureThirstMultiplier;
    GetTemperatureMultipliersFromActiveEffects(TemperatureHungerMultiplier, TemperatureThirstMultiplier);
    
    // Calculate final decay values: base rate * (1 + all modifiers) * temperature multiplier
    // Formula: FinalDecay = BaseRate * (1 + TemperatureMod + ActivityMod) * StatusEffectMultiplier
    float CalculatedHungerDecay = EffectiveHungerBase * (1.f + HungerActivityMod + HungerTemperatureMod) * TemperatureHungerMultiplier;
    float CalculatedThirstDecay = EffectiveThirstBase * (1.f + ThirstActivityMod + ThirstTemperatureMod) * TemperatureThirstMultiplier;
    
    // Apply debug multiplier for testing/balancing purposes
    // Designers can speed up/slow down decay for testing without changing base values
    CalculatedHungerDecay *= GetConfig()->GetDebugDecayMultiplier();
    CalculatedThirstDecay *= GetConfig()->GetDebugDecayMultiplier();
    
    // Ensure decay values are never negative (could happen with extreme negative modifiers)
    // Negative decay would heal the player, which might not be intended
    CalculatedHungerDecay = FMath::Max(0.f, CalculatedHungerDecay);
    CalculatedThirstDecay = FMath::Max(0.f, CalculatedThirstDecay);
    
    // Broadcast computed decay values for UI/analytics systems
    // UI can show current decay rates, analytics can track balance issues
    OnDecaysComputed.Broadcast(CalculatedHungerDecay, CalculatedThirstDecay);
    
    // Apply calculated decay to actual stat values in the statistics system
    ApplyDecayToStats(CalculatedHungerDecay, CalculatedThirstDecay);
    
    // Evaluate all survival state transitions and fire appropriate events
    // Order matters here - state transitions should happen before status effects
    EvaluateSurvivalStateTransitions(CachedValues);
    
    // NEW: Apply data-driven survival status effects based on current conditions
    EvaluateAndApplySurvivalEffects(CachedValues);
    
    // Continue with existing systems
    EvaluateWeatherHazards(CachedValues);
    
    // Update body temperature simulation based on environmental conditions
    UpdateBodyTemperature(PlayerLocationTemperature, CachedValues);
    
    // Fire periodic warning events if conditions are met
    // These use escalating frequency system for increased urgency over time
    MaybeFireStarvationWarning(TimeOfDay, CachedValues.Hunger);
    MaybeFireDehydrationWarning(TimeOfDay, CachedValues.Thirst);
    MaybeFireHeatstrokeWarning(TimeOfDay, CachedValues.BodyTemp);
    MaybeFireHypothermiaWarning(TimeOfDay, CachedValues.BodyTemp);
    
    // Update overall survival state based on current conditions
    // RENAMED: This is now clearly UI-only function
    UpdateSurvivalUIState(CachedValues);

#if !UE_BUILD_SHIPPING
    // Debug logging for development builds only - helps track simulation state
    UE_LOG(LogTemp, VeryVerbose, TEXT("Survival Tick - BodyTemp: %.2f, Ambient: %.2f, Hunger: %.2f, Thirst: %.2f"),
        CachedValues.BodyTemp, PlayerLocationTemperature, CachedValues.Hunger, CachedValues.Thirst);
#endif
}

// ======== Core Helper Functions ========

UNomadSurvivalNeedsComponent::FCachedStatValues UNomadSurvivalNeedsComponent::GetCachedStatValues() const
{
    // Initialize return structure with default values
    FCachedStatValues Values;
    
    // Only proceed if both required components are available
    if (StatisticsComponent && GetConfig())
    {
        // Retrieve all required stat values in one batch to minimize component calls
        // This is a significant performance optimization for per-minute ticks
        Values.Hunger = StatisticsComponent->GetCurrentValueForStatitstic(GetConfig()->GetHungerStatTag());
        Values.Thirst = StatisticsComponent->GetCurrentValueForStatitstic(GetConfig()->GetThirstStatTag());
        Values.BodyTemp = StatisticsComponent->GetCurrentValueForStatitstic(GetConfig()->GetBodyTempStatTag());
        Values.Endurance = StatisticsComponent->GetCurrentPrimaryAttributeValue(GetConfig()->GetEnduranceStatTag());
        
        // Mark as valid so calling code knows the data is reliable
        Values.bValid = true;
    }
    
    // Return cached values (will be invalid if components missing)
    return Values;
}

// Add this helper function to the component
void UNomadSurvivalNeedsComponent::GetTemperatureMultipliersFromActiveEffects(float& OutHungerMultiplier, float& OutThirstMultiplier) const
{
    OutHungerMultiplier = 1.0f;
    OutThirstMultiplier = 1.0f;
    
    if (!StatusEffectManagerComponent || !GetConfig()) return;
    
    const TArray<FActiveEffect>& ActiveEffects = StatusEffectManagerComponent->GetActiveEffects();
    
    for (const FActiveEffect& ActiveEffect : ActiveEffects)
    {
        if (const UNomadSurvivalStatusEffect* SurvivalEffect = Cast<UNomadSurvivalStatusEffect>(ActiveEffect.EffectInstance))
        {
            ESurvivalSeverity Severity = SurvivalEffect->GetSeverityLevel();
            
            // Get effect tag from config
            FGameplayTag EffectTag;
            if (const UNomadInfiniteEffectConfig* EffectConfig = SurvivalEffect->GetEffectConfig())
            {
                EffectTag = EffectConfig->EffectTag;
            }
            
            // Parent tag matching for more flexible tag hierarchies
            static const FGameplayTag HypothermiaParentTag = FGameplayTag::RequestGameplayTag("Status.Survival.Hypothermia");
            static const FGameplayTag HeatstrokeParentTag = FGameplayTag::RequestGameplayTag("Status.Survival.Heatstroke");
            
            if (EffectTag.MatchesTag(HypothermiaParentTag))
            {
                // Apply cold multiplier based on severity
                switch (Severity)
                {
                    case ESurvivalSeverity::Mild:
                        OutHungerMultiplier = FMath::Max(OutHungerMultiplier, GetConfig()->GetColdMildHungerMultiplier());
                        break;
                    case ESurvivalSeverity::Heavy:
                        OutHungerMultiplier = FMath::Max(OutHungerMultiplier, GetConfig()->GetColdSevereHungerMultiplier());
                        break;
                    case ESurvivalSeverity::Extreme:
                        OutHungerMultiplier = FMath::Max(OutHungerMultiplier, GetConfig()->GetColdExtremeHungerMultiplier());
                        break;
                    default:
                        break;
                }
            }
            else if (EffectTag.MatchesTag(HeatstrokeParentTag))
            {
                // Apply heat multiplier based on severity
                switch (Severity)
                {
                    case ESurvivalSeverity::Mild:
                        OutThirstMultiplier = FMath::Max(OutThirstMultiplier, GetConfig()->GetHeatMildThirstMultiplier());
                        break;
                    case ESurvivalSeverity::Heavy:
                        OutThirstMultiplier = FMath::Max(OutThirstMultiplier, GetConfig()->GetHeatSevereThirstMultiplier());
                        break;
                    case ESurvivalSeverity::Extreme:
                        OutThirstMultiplier = FMath::Max(OutThirstMultiplier, GetConfig()->GetHeatExtremeThirstMultiplier());
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

float UNomadSurvivalNeedsComponent::ComputeNormalizedTemperature(const float InRawTemperature, const bool bIsWarmBar) const
{
    // Early exit if config is missing
    if (!GetConfig()) return 0.f;
    
    // Get temperature ranges based on current temperature unit setting
    // This allows the same config to work with both Celsius and Fahrenheit inputs
    const float MinT = (TemperatureUnit == ETemperatureUnit::Celsius)
        ? GetConfig()->GetMinExternalTempCelsius()
        : GetConfig()->GetMinExternalTempFahrenheit();
    const float MaxT = (TemperatureUnit == ETemperatureUnit::Celsius)
        ? GetConfig()->GetMaxExternalTempCelsius()
        : GetConfig()->GetMaxExternalTempFahrenheit();
    const float NeutralT = (TemperatureUnit == ETemperatureUnit::Celsius) ? 0.f : 32.f;

    // For cold bar: fills as temp goes below neutral (0°C/32°F)
    // Used for UI cold indicators - higher bar = colder temperature
    if (!bIsWarmBar && InRawTemperature < NeutralT)
    {
        const float ClampedTemp = FMath::Clamp(InRawTemperature, MinT, NeutralT);
        const float Normalized = (NeutralT - ClampedTemp) / (NeutralT - MinT);
        return FMath::Clamp(Normalized * GetConfig()->GetExternalTemperatureScale(), 0.f, 1.f);
    }
    // For warm bar: fills as temp goes above neutral (0°C/32°F)  
    // Used for UI heat indicators - higher bar = hotter temperature
    else if (bIsWarmBar && InRawTemperature > NeutralT)
    {
        const float ClampedTemp = FMath::Clamp(InRawTemperature, NeutralT, MaxT);
        const float Normalized = (ClampedTemp - NeutralT) / (MaxT - NeutralT);
        return FMath::Clamp(Normalized * GetConfig()->GetExternalTemperatureScale(), 0.f, 1.f);
    }
    
    // At neutral temp, both bars are 0 (comfortable temperature)
    return 0.f;
}

float UNomadSurvivalNeedsComponent::ComputeColdHungerModifier(const float NormalizedTempForCurve) const
{
    // Early exit if config is missing
    if (!GetConfig()) return 0.f;
    
    // Use designer curve to determine hunger modifier based on temperature
    // Cold temperatures typically increase hunger (body burns more calories for warmth)
    if (const UCurveFloat* Curve = GetConfig()->AdvancedModifierCurves.HungerDecayByTemperatureCurve)
    {
        // Subtract 1 because curve returns multiplier (1.0 = no change, 1.5 = 50% increase)
        // We want modifier (0.0 = no change, 0.5 = 50% increase)
        return Curve->GetFloatValue(NormalizedTempForCurve) - 1.f;
    }
    
    // No modifier if curve is not configured
    return 0.f;
}

float UNomadSurvivalNeedsComponent::ComputeHotThirstModifier(const float NormalizedTempForCurve) const
{
    // Early exit if config is missing
    if (!GetConfig()) return 0.f;
    
    // Use designer curve to determine thirst modifier based on temperature
    // Hot temperatures typically increase thirst (body loses more water through sweating)
    if (const UCurveFloat* Curve = GetConfig()->AdvancedModifierCurves.ThirstDecayByTemperatureCurve)
    {
        // Subtract 1 because curve returns multiplier (1.0 = no change, 1.5 = 50% increase)
        // We want modifier (0.0 = no change, 0.5 = 50% increase)
        return Curve->GetFloatValue(NormalizedTempForCurve) - 1.f;
    }
    
    // No modifier if curve is not configured
    return 0.f;
}

float UNomadSurvivalNeedsComponent::ComputeHungerActivityModifier(const float NormalizedActivity) const
{
    // Early exit if config is missing
    if (!GetConfig()) return 0.f;
    
    // Use designer curve to determine hunger modifier based on activity level
    // Higher activity (running, sprinting) increases hunger due to energy expenditure
    if (const UCurveFloat* Curve = GetConfig()->AdvancedModifierCurves.HungerDecayByActivityCurve)
    {
        // Subtract 1 because curve returns multiplier (1.0 = no change, 2.0 = 100% increase)
        // We want modifier (0.0 = no change, 1.0 = 100% increase)
        return Curve->GetFloatValue(NormalizedActivity) - 1.f;
    }
    
    // No modifier if curve is not configured
    return 0.f;
}

float UNomadSurvivalNeedsComponent::ComputeThirstActivityModifier(const float NormalizedActivity) const
{
    // Early exit if config is missing
    if (!GetConfig()) return 0.f;
    
    // Use designer curve to determine thirst modifier based on activity level
    // Higher activity (running, sprinting) increases thirst due to increased respiration and sweating
    if (const UCurveFloat* Curve = GetConfig()->AdvancedModifierCurves.ThirstDecayByActivityCurve)
    {
        // Subtract 1 because curve returns multiplier (1.0 = no change, 2.0 = 100% increase)
        // We want modifier (0.0 = no change, 1.0 = 100% increase)
        return Curve->GetFloatValue(NormalizedActivity) - 1.f;
    }
    
    // No modifier if curve is not configured
    return 0.f;
}

void UNomadSurvivalNeedsComponent::ApplyDecayToStats(const float InHungerDecay, const float InThirstDecay) const
{
    // Early exit if required components are missing
    if (!StatisticsComponent || !GetConfig()) return;
    
    // Apply negative values to reduce stats (decay)
    // FMath::Max ensures we never apply negative decay (which would heal the player)
    StatisticsComponent->ModifyStatistic(GetConfig()->GetHungerStatTag(), -FMath::Max(0.f, InHungerDecay));
    StatisticsComponent->ModifyStatistic(GetConfig()->GetThirstStatTag(), -FMath::Max(0.f, InThirstDecay));
}

FString UNomadSurvivalNeedsComponent::GetTemperatureNotificationText(const FGameplayTag& EffectTag, const UNomadSurvivalNeedsData* Config) const
{
    if (!Config) return TEXT("");
    
    // Heatstroke notifications
    if (EffectTag.MatchesTagExact(FGameplayTag::RequestGameplayTag("Status.Survival.Heatstroke.Extreme")))
    {
        return FString::Printf(TEXT("EXTREME HEAT - Thirst x%.0f Faster!"), Config->GetHeatExtremeThirstMultiplier());
    }
    else if (EffectTag.MatchesTagExact(FGameplayTag::RequestGameplayTag("Status.Survival.Heatstroke.Severe")))
    {
        return FString::Printf(TEXT("SEVERE HEAT - Thirst x%.0f Faster!"), Config->GetHeatSevereThirstMultiplier());
    }
    else if (EffectTag.MatchesTagExact(FGameplayTag::RequestGameplayTag("Status.Survival.Heatstroke.Mild")))
    {
        return FString::Printf(TEXT("Getting Hot - Thirst x%.0f Faster"), Config->GetHeatMildThirstMultiplier());
    }
    // Hypothermia notifications
    else if (EffectTag.MatchesTagExact(FGameplayTag::RequestGameplayTag("Status.Survival.Hypothermia.Extreme")))
    {
        return FString::Printf(TEXT("EXTREME COLD - Hunger x%.0f Faster!"), Config->GetColdExtremeHungerMultiplier());
    }
    else if (EffectTag.MatchesTagExact(FGameplayTag::RequestGameplayTag("Status.Survival.Hypothermia.Severe")))
    {
        return FString::Printf(TEXT("SEVERE COLD - Hunger x%.0f Faster!"), Config->GetColdSevereHungerMultiplier());
    }
    else if (EffectTag.MatchesTagExact(FGameplayTag::RequestGameplayTag("Status.Survival.Hypothermia.Mild")))
    {
        return FString::Printf(TEXT("Getting Cold - Hunger x%.0f Faster"), Config->GetColdMildHungerMultiplier());
    }
    
    return TEXT("Temperature Effect Applied");
}

FLinearColor UNomadSurvivalNeedsComponent::GetSeverityColor(ESurvivalSeverity Severity) const
{
    switch (Severity)
    {
        case ESurvivalSeverity::Mild:
            return FLinearColor::Yellow;
        case ESurvivalSeverity::Heavy:
            return FLinearColor(1.0f, 0.5f, 0.0f); // Orange
        case ESurvivalSeverity::Severe:
        case ESurvivalSeverity::Extreme:
            return FLinearColor::Red;
        default:
            return FLinearColor::White;
    }
}

// ======== Event System Functions ========

void UNomadSurvivalNeedsComponent::EvaluateSurvivalStateTransitions(const FCachedStatValues& CachedValues)
{
    // --- Starvation state management (NO warning broadcasts here) ---
    // Starvation occurs when hunger reaches 0 - most severe hunger state
    if (IsStarving(CachedValues.Hunger))
    {
        // Apply status effect every tick while starving (effect should be non-stacking)
        ApplyGenericStatusEffect(GetConfig()->GetStarvationDebuffEffect(), GetConfig()->StarvationHealthDoTPercent);
        
        // Fire transition event only once when entering starvation state
        if (!bIsStarving)
        {
            bIsStarving = true;
            UE_LOG_SURVIVAL_EVENTS(Log, TEXT("Player started starving - Hunger: %.2f"), CachedValues.Hunger);
            OnStarvationStarted.Broadcast(CachedValues.Hunger);
        }
    }
    // Recovery from starvation when hunger goes above 0
    else if (bIsStarving)
    {
        bIsStarving = false;
        UE_LOG_SURVIVAL_EVENTS(Log, TEXT("Player recovered from starvation - Hunger: %.2f"), CachedValues.Hunger);
        OnStarvationEnded.Broadcast(CachedValues.Hunger);
        // Remove starvation debuff effect when recovering
        TryRemoveStatusEffect(GetConfig()->StarvationDebuffTag);
    }

    // --- Dehydration state management (NO warning broadcasts here) ---
    // Dehydration occurs when thirst reaches 0 - most severe thirst state
    if (IsDehydrated(CachedValues.Thirst))
    {
        // Apply status effect every tick while dehydrated (effect should be non-stacking)
        ApplyGenericStatusEffect(GetConfig()->GetDehydrationDebuffEffect(), GetConfig()->DehydrationHealthDoTPercent);
        
        // Fire transition event only once when entering dehydration state
        if (!bIsDehydrated)
        {
            bIsDehydrated = true;
            UE_LOG_SURVIVAL_EVENTS(Log, TEXT("Player started getting dehydrated - Thirst: %.2f"), CachedValues.Thirst);
            OnDehydrationStarted.Broadcast(CachedValues.Thirst);
        }
    }
    // Recovery from dehydration when thirst goes above 0
    else if (bIsDehydrated)
    {
        bIsDehydrated = false;
        UE_LOG_SURVIVAL_EVENTS(Log, TEXT("Player recovered from dehydration - Thirst: %.2f"), CachedValues.Thirst);
        OnDehydrationEnded.Broadcast(CachedValues.Thirst);
        // Remove dehydration debuff effect when recovering
        TryRemoveStatusEffect(GetConfig()->DehydrationDebuffTag);
    }

    // NOTE: Warning broadcasts are now handled exclusively by MaybeFireXXXWarning() functions
    // This prevents duplicate warnings while allowing periodic reminders with escalation
}

void UNomadSurvivalNeedsComponent::EvaluateWeatherHazards(const FCachedStatValues& CachedValues)
{
    // Early exit if data is invalid or config is missing
    if (!CachedValues.bValid || !GetConfig()) return;

    // NOTE: Warning broadcasts for temperature hazards are now handled by
    // MaybeFireHeatstrokeWarning() and MaybeFireHypothermiaWarning()
    // This section only manages internal state flags for the warning system
    
    // --- Heatstroke warning state management ---
    // Get threshold values from config for heatstroke warning zone calculation
    const float HeatstrokeThreshold = GetConfig()->GetHeatstrokeThreshold();
    const float HeatstrokeWarningDelta = GetConfig()->GetHeatstrokeWarningDelta();
    
    // Check if player is in heatstroke warning zone (approaching but not yet at heatstroke)
    if (CachedValues.BodyTemp >= HeatstrokeThreshold - HeatstrokeWarningDelta && 
        CachedValues.BodyTemp < HeatstrokeThreshold)
    {
        // Set warning flag when entering warning zone
        if (!bHeatstrokeWarningGiven)
        {
            bHeatstrokeWarningGiven = true;
            // Actual warning broadcast happens in MaybeFireHeatstrokeWarning()
        }
    }
    // Clear warning flag when exiting warning zone (body temp drops significantly)
    else if (CachedValues.BodyTemp < HeatstrokeThreshold - HeatstrokeWarningDelta)
    {
        bHeatstrokeWarningGiven = false;
    }

    // --- Hypothermia warning state management ---
    // Get threshold values from config for hypothermia warning zone calculation
    const float HypothermiaThreshold = GetConfig()->GetHypothermiaThreshold();
    const float HypothermiaWarningDelta = GetConfig()->GetHypothermiaWarningDelta();
    
    // Check if player is in hypothermia warning zone (approaching but not yet at hypothermia)
    if (CachedValues.BodyTemp <= HypothermiaThreshold + HypothermiaWarningDelta && 
        CachedValues.BodyTemp > HypothermiaThreshold)
    {
        // Set warning flag when entering warning zone
        if (!bHypothermiaWarningGiven)
        {
            bHypothermiaWarningGiven = true;
            // Actual warning broadcast happens in MaybeFireHypothermiaWarning()
        }
    }
    // Clear warning flag when exiting warning zone (body temp rises significantly)
    else if (CachedValues.BodyTemp > HypothermiaThreshold + HypothermiaWarningDelta)
    {
        bHypothermiaWarningGiven = false;
    }
}

void UNomadSurvivalNeedsComponent::UpdateBodyTemperature(const float AmbientTempCelsius, const FCachedStatValues& CachedValues)
{
    // Early exit if required components/data are missing
    if (!StatisticsComponent || !GetConfig() || !CachedValues.bValid) return;

    // Log body temperature update for debugging temperature simulation
    UE_LOG_SURVIVAL_TEMP(VeryVerbose, TEXT("Updating body temperature - Ambient: %.2f, Current: %.2f"), 
                        AmbientTempCelsius, CachedValues.BodyTemp);

    const float CurrentBodyTemp = CachedValues.BodyTemp;

    // --- Safe Zone: If ambient temp is inside safe zone, body temp trends to normal ---
    // Safe zone represents comfortable environmental conditions where body can self-regulate
    if (AmbientTempCelsius >= GetConfig()->GetSafeAmbientTempMinC() && 
        AmbientTempCelsius <= GetConfig()->GetSafeAmbientTempMaxC())
    {
        // Calculate how far current body temp is from normal (37°C typically)
        const float TempDifference = GetConfig()->NormalBodyTemperature - CurrentBodyTemp;
        
        // Apply proportional adjustment toward normal temperature
        const float ProportionalChange = TempDifference * GetConfig()->AdvancedBodyTempParams.BodyTempAdjustRate;
        
        // Clamp change rate to prevent unrealistic temperature swings
        float ClampedChange = FMath::Clamp(ProportionalChange, 
            -GetConfig()->AdvancedBodyTempParams.MaxBodyTempChangeRate, 
            GetConfig()->AdvancedBodyTempParams.MaxBodyTempChangeRate);

        // Ensure minimum change rate if there's a significant difference
        // This prevents the system from getting "stuck" with tiny changes
        if (FMath::Abs(ClampedChange) < GetConfig()->AdvancedBodyTempParams.MinBodyTempChangeRate && 
            FMath::Abs(TempDifference) > KINDA_SMALL_NUMBER)
        {
            ClampedChange = FMath::Sign(TempDifference) * GetConfig()->AdvancedBodyTempParams.MinBodyTempChangeRate;
        }

        // Apply temperature change if it's significant enough
        if (FMath::Abs(ClampedChange) > KINDA_SMALL_NUMBER)
        {
            StatisticsComponent->ModifyStatistic(GetConfig()->GetBodyTempStatTag(), ClampedChange);
        }
    }
    else
    {
        // --- Outside safe zone: Use curve for non-linear drift ---
        // When environment is too hot/cold, body temp drifts toward ambient temperature
        
        // Get curve multiplier for non-linear temperature effects
        // Extreme temperatures have disproportionately large effects
        float CurveMultiplier = 1.f;
        if (GetConfig()->GetBodyTempDriftCurve())
        {
            CurveMultiplier = GetConfig()->GetBodyTempDriftCurve()->GetFloatValue(AmbientTempCelsius);
        }

        // Calculate drift toward ambient temperature (not toward normal)
        const float TempDifference = AmbientTempCelsius - CurrentBodyTemp;
        
        // Apply environmental drift with curve modification
        const float ProportionalChange = TempDifference * GetConfig()->AdvancedBodyTempParams.BodyTempAdjustRate * CurveMultiplier;
        
        // Clamp change rate to prevent unrealistic temperature swings
        float ClampedChange = FMath::Clamp(ProportionalChange, 
            -GetConfig()->AdvancedBodyTempParams.MaxBodyTempChangeRate, 
            GetConfig()->AdvancedBodyTempParams.MaxBodyTempChangeRate);

        // Ensure minimum change rate if there's a significant difference
        if (FMath::Abs(ClampedChange) < GetConfig()->AdvancedBodyTempParams.MinBodyTempChangeRate && 
            FMath::Abs(TempDifference) > KINDA_SMALL_NUMBER)
        {
            ClampedChange = FMath::Sign(TempDifference) * GetConfig()->AdvancedBodyTempParams.MinBodyTempChangeRate;
        }

        // Apply temperature change if it's significant enough
        if (FMath::Abs(ClampedChange) > KINDA_SMALL_NUMBER)
        {
            StatisticsComponent->ModifyStatistic(GetConfig()->GetBodyTempStatTag(), ClampedChange);
        }
    }

    // Get updated body temperature after modification for hazard evaluation
    const float UpdatedBodyTemp = StatisticsComponent->GetCurrentValueForStatitstic(GetConfig()->GetBodyTempStatTag());

    // --- Heatstroke hazard state ---
    // Heatstroke requires sustained high body temperature, not just momentary spikes
    if (IsHeatstroke(UpdatedBodyTemp) && !bInHeatstroke)
    {
        // Increment exposure counter - heatstroke requires sustained exposure
        HeatExposureCounter++;
        
        // Check if player has been in heatstroke zone long enough to trigger effect
        if (HeatExposureCounter >= GetConfig()->GetHeatstrokeDurationMinutes())
        {
            bInHeatstroke = true;
            // Broadcast heatstroke started event for gameplay systems
            OnHeatstrokeStarted.Broadcast(UpdatedBodyTemp);
            // Apply heatstroke status effect (health drain, movement penalty, etc.)
            ApplyGenericStatusEffect(GetConfig()->GetHeatstrokeDebuffEffect(), 1);
        }
    }
    // Recovery from heatstroke when body temperature drops below threshold
    else if (!IsHeatstroke(UpdatedBodyTemp) && bInHeatstroke)
    {
        bInHeatstroke = false;
        HeatExposureCounter = 0; // Reset exposure counter on recovery
        // Broadcast heatstroke ended event for gameplay systems
        OnHeatstrokeEnded.Broadcast(UpdatedBodyTemp);
        // Remove heatstroke status effect
        TryRemoveStatusEffect(GetConfig()->HeatstrokeDebuffTag);
    }
    // Reset counter if no longer in heatstroke zone (prevents accumulation across non-consecutive exposures)
    else if (!IsHeatstroke(UpdatedBodyTemp))
    {
        HeatExposureCounter = 0;
    }

    // --- Hypothermia hazard state ---
    // Hypothermia requires sustained low body temperature, not just momentary drops
    if (IsHypothermic(UpdatedBodyTemp) && !bInHypothermia)
    {
        // Increment exposure counter - hypothermia requires sustained exposure
        ColdExposureCounter++;
        
        // Check if player has been in hypothermia zone long enough to trigger effect
        if (ColdExposureCounter >= GetConfig()->GetHypothermiaDurationMinutes())
        {
            bInHypothermia = true;
            // Broadcast hypothermia started event for gameplay systems
            OnHypothermiaStarted.Broadcast(UpdatedBodyTemp);
            // Apply hypothermia status effect (movement penalty, reduced stamina regen, etc.)
            ApplyGenericStatusEffect(GetConfig()->GetHypothermiaDebuffEffect(), 1);
        }
    }
    // Recovery from hypothermia when body temperature rises above threshold
    else if (!IsHypothermic(UpdatedBodyTemp) && bInHypothermia)
    {
        bInHypothermia = false;
        ColdExposureCounter = 0; // Reset exposure counter on recovery
        // Broadcast hypothermia ended event for gameplay systems
        OnHypothermiaEnded.Broadcast(UpdatedBodyTemp);
        // Remove hypothermia status effect
        TryRemoveStatusEffect(GetConfig()->HypothermiaDebuffTag);
    }
    // Reset counter if no longer in hypothermia zone (prevents accumulation across non-consecutive exposures)
    else if (!IsHypothermic(UpdatedBodyTemp))
    {
        ColdExposureCounter = 0;
    }
}

void UNomadSurvivalNeedsComponent::UpdateSurvivalUIState(const FCachedStatValues& CachedValues)
{
    // Early exit if cached values are invalid
    if (!CachedValues.bValid) return;
    
    // Start with normal state as default
    ESurvivalState NewState = ESurvivalState::Normal;

    // Priority order: most severe conditions first to ensure proper state precedence
    // Temperature hazards take the highest priority as they're immediately life-threatening
    if (IsHeatstroke(CachedValues.BodyTemp))
        NewState = ESurvivalState::Heatstroke;
    else if (IsHypothermic(CachedValues.BodyTemp))
        NewState = ESurvivalState::Hypothermic;
    // Critical hunger/thirst states (at 0) take next priority
    else if (IsStarving(CachedValues.Hunger))
        NewState = ESurvivalState::Starving;
    else if (IsDehydrated(CachedValues.Thirst))
        NewState = ESurvivalState::Dehydrated;
    // Warning hunger/thirst states (low but not 0) have lowest priority
    else if (IsHungry(CachedValues.Hunger))
        NewState = ESurvivalState::Hungry;
    else if (IsThirsty(CachedValues.Thirst))
        NewState = ESurvivalState::Thirsty;
    
    // Only broadcast state change if the state actually changed
    // This prevents unnecessary network traffic and event spam
    if (NewState != CurrentSurvivalState)
    {
        const ESurvivalState OldState = CurrentSurvivalState;
        CurrentSurvivalState = NewState; // Update replicated state for clients
        
        // Broadcast state change event for UI systems and gameplay logic
        OnSurvivalStateChanged.Broadcast(OldState, NewState);
    }
}

// ======== New Survival Status Effect System ========

void UNomadSurvivalNeedsComponent::EvaluateAndApplySurvivalEffects(const FCachedStatValues& CachedValues)
{
    /*
    -----------------------------------------------------------------------------
    EvaluateAndApplySurvivalEffects
    -----------------------------------------------------------------------------
    Purpose:
        Replaces the old manual attribute manipulation system with proper status effects.
        Evaluates current survival conditions (hunger, thirst, temperature) and applies
        appropriate status effects with data-driven attribute modifiers.

    New Approach:
        - Uses UNomadSurvivalStatusEffect classes instead of manual GUID tracking
        - All attribute modifiers come from UNomadInfiniteEffectConfig data assets
        - Status effects handle their own application/removal and visual effects
        - Cleaner, more maintainable, and fully data-driven
    -----------------------------------------------------------------------------
    */

    // Early exit if cached values are invalid
    if (!CachedValues.bValid) return;

    // Get status effect manager for applying effects
    if (!StatusEffectManagerComponent)
    {
        UE_LOG_SURVIVAL(Warning, TEXT("No StatusEffectManager found on %s - survival effects disabled"), 
               *GetOwner()->GetName());
        return;
    }

    // Evaluate each survival condition and apply appropriate effects
    EvaluateHungerEffects(CachedValues.Hunger);
    EvaluateThirstEffects(CachedValues.Thirst);  
    EvaluateTemperatureEffects(CachedValues.BodyTemp);
}

void UNomadSurvivalNeedsComponent::EvaluateHungerEffects(float HungerLevel)
{
    const UNomadSurvivalNeedsData* Config = GetConfig();
        if (!Config || !StatusEffectManagerComponent) return;
    
        // Get max hunger to calculate percentage
        const float MaxHunger = StatisticsComponent->GetMaxValueForStatitstic(Config->GetHungerStatTag());
        const float HungerPercent = (MaxHunger > 0.f) ? (HungerLevel / MaxHunger) : 0.f;
    
        // Define effect tags for state tracking
        static const FGameplayTag MildTag = FGameplayTag::RequestGameplayTag("Status.Survival.Starvation.Mild");
        static const FGameplayTag SevereTag = FGameplayTag::RequestGameplayTag("Status.Survival.Starvation.Severe");
        
        // Determine what effects SHOULD be active (business logic)
        const bool bShouldHaveSevere = (HungerLevel <= 0.0f);
        const bool bShouldHaveMild = (!bShouldHaveSevere && HungerPercent < Config->GetHungerMildThreshold());
        
        // Check what effects ARE currently active (system state)
        const bool bHasMild = StatusEffectManagerComponent->HasStatusEffect(MildTag);
        const bool bHasSevere = StatusEffectManagerComponent->HasStatusEffect(SevereTag);
        
        // STATE MANAGEMENT: Only apply/remove when state actually changes
        
        // Remove effects that shouldn't be active
        if (bHasMild && !bShouldHaveMild)
        {
            StatusEffectManagerComponent->Nomad_RemoveStatusEffect(MildTag);
            UE_LOG_SURVIVAL(Log, TEXT("Removed mild starvation effect - hunger improved"));
        }
        if (bHasSevere && !bShouldHaveSevere)
        {
            StatusEffectManagerComponent->Nomad_RemoveStatusEffect(SevereTag);
            UE_LOG_SURVIVAL(Log, TEXT("Removed severe starvation effect - hunger improved"));
        }
        
        // Apply effects that should be active (ONLY if not already active)
        if (bShouldHaveSevere && !bHasSevere)
        {
            // SEVERE: All penalties + DoT configured in status effect config
            ApplyStatusEffect(
                Config->GetStarvationSevereEffectClass(), 
                ESurvivalSeverity::Severe,
                Config->StarvationHealthDoTPercent  // Only DoT from config
            );
            
            // Movement/stamina penalties are handled by the status effect's PersistentAttributeModifier
            BroadcastSurvivalNotification(
                TEXT("🍽️ STARVING - Taking Damage!"), 
                FLinearColor::Red, 
                5.0f
            );
            UE_LOG_SURVIVAL(Log, TEXT("Applied severe starvation effect with DoT"));
        }
        else if (bShouldHaveMild && !bHasMild)
        {
            // MILD: Movement/stamina penalties configured in status effect config
            ApplyStatusEffect(
                Config->GetStarvationMildEffectClass(),
                ESurvivalSeverity::Mild,
                0.0f  // No DoT for mild level
            );
            
            // All penalties (movement speed, stamina cap) handled by PersistentAttributeModifier
            BroadcastSurvivalNotification(
                TEXT("🍽️ Hungry - Performance Reduced"), 
                FLinearColor::Yellow, 
                4.0f
            );
            UE_LOG_SURVIVAL(Log, TEXT("Applied mild starvation effect"));
        }
}

void UNomadSurvivalNeedsComponent::EvaluateThirstEffects(float ThirstLevel)
{
    const UNomadSurvivalNeedsData* Config = GetConfig();
        if (!Config || !StatusEffectManagerComponent) return;
    
        // Get max thirst to calculate percentage
        const float MaxThirst = StatisticsComponent->GetMaxValueForStatitstic(Config->GetThirstStatTag());
        const float ThirstPercent = (MaxThirst > 0.f) ? (ThirstLevel / MaxThirst) : 0.f;
    
        // Define effect tags for state tracking
        static const FGameplayTag MildTag = FGameplayTag::RequestGameplayTag("Status.Survival.Dehydration.Mild");
        static const FGameplayTag SevereTag = FGameplayTag::RequestGameplayTag("Status.Survival.Dehydration.Severe");
    
        // Determine what effects SHOULD be active
        const bool bShouldHaveSevere = (ThirstLevel <= 0.0f);
        const bool bShouldHaveMild = (!bShouldHaveSevere && ThirstPercent < Config->GetThirstMildThreshold());
        
        // Check what effects ARE currently active
        const bool bHasMild = StatusEffectManagerComponent->HasStatusEffect(MildTag);
        const bool bHasSevere = StatusEffectManagerComponent->HasStatusEffect(SevereTag);
        
        // STATE MANAGEMENT: Only change when state transitions occur
        
        // Remove effects that shouldn't be active
        if (bHasMild && !bShouldHaveMild)
        {
            StatusEffectManagerComponent->Nomad_RemoveStatusEffect(MildTag);
            UE_LOG_SURVIVAL(Log, TEXT("Removed mild dehydration effect - thirst improved"));
        }
        if (bHasSevere && !bShouldHaveSevere)
        {
            StatusEffectManagerComponent->Nomad_RemoveStatusEffect(SevereTag);
            UE_LOG_SURVIVAL(Log, TEXT("Removed severe dehydration effect - thirst improved"));
        }
        
        // Apply effects that should be active (ONLY if not already active)
        if (bShouldHaveSevere && !bHasSevere)
        {
            // SEVERE: Stamina penalties + DoT configured in status effect
            ApplyStatusEffect(
                Config->GetDehydrationSevereEffectClass(),
                ESurvivalSeverity::Severe, 
                Config->DehydrationHealthDoTPercent  // Only DoT from config
            );
            
            // Movement/stamina penalties handled by PersistentAttributeModifier in config
            BroadcastSurvivalNotification(
                TEXT("💧 DEHYDRATED - Taking Damage!"), 
                FLinearColor::Red, 
                5.0f
            );
            UE_LOG_SURVIVAL(Log, TEXT("Applied severe dehydration effect with DoT"));
        }
        else if (bShouldHaveMild && !bHasMild)
        {
            // MILD: Stamina penalties configured in status effect
            ApplyStatusEffect(
                Config->GetDehydrationMildEffectClass(),
                ESurvivalSeverity::Mild,
                0.0f  // No health damage at mild level
            );
            
            // All penalties handled by status effect's PersistentAttributeModifier
            BroadcastSurvivalNotification(
                TEXT("💧 Thirsty - Performance Reduced"), 
                FLinearColor::Blue, 
                4.0f
            );
            UE_LOG_SURVIVAL(Log, TEXT("Applied mild dehydration effect"));
        }
}

void UNomadSurvivalNeedsComponent::EvaluateTemperatureEffects(float BodyTemp)
{
    const UNomadSurvivalNeedsData* Config = GetConfig();
    if (!Config || !StatusEffectManagerComponent) return;

    // Temperature effect tags hierarchy for precise state management
    static const FGameplayTag HeatstrokeParent = FGameplayTag::RequestGameplayTag("Status.Survival.Heatstroke");
    static const FGameplayTag HypothermiaParent = FGameplayTag::RequestGameplayTag("Status.Survival.Hypothermia");
    
    // Determine target temperature effect based on precise thresholds
    FGameplayTag TargetEffectTag;
    TSubclassOf<UNomadSurvivalStatusEffect> TargetEffectClass = nullptr;
    ESurvivalSeverity TargetSeverity = ESurvivalSeverity::None;
    FString NotificationText;
    FLinearColor NotifColor = FLinearColor::White;

    // HEATSTROKE EFFECTS (matches image requirements)
    if (BodyTemp >= Config->GetHeatstrokeExtremeThreshold())
    {
        // EXTREME: Thirst X4 Faster + %30 Movement Slow (from config)
        TargetEffectTag = FGameplayTag::RequestGameplayTag("Status.Survival.Heatstroke.Extreme");
        TargetEffectClass = Config->GetHeatstrokeExtremeEffectClass();
        TargetSeverity = ESurvivalSeverity::Extreme;
        NotificationText = FString::Printf(TEXT("🔥 EXTREME HEAT - Thirst x%.0f, Movement %d%% Slower!"), 
                                         Config->GetHeatExtremeThirstMultiplier(), 30);
        NotifColor = FLinearColor::Red;
    }
    else if (BodyTemp >= Config->GetHeatstrokeHeavyThreshold())
    {
        // HEAVY: Thirst X3 Faster + %20 Movement Slow (from config)
        TargetEffectTag = FGameplayTag::RequestGameplayTag("Status.Survival.Heatstroke.Severe");
        TargetEffectClass = Config->GetHeatstrokeSevereEffectClass();
        TargetSeverity = ESurvivalSeverity::Heavy;
        NotificationText = FString::Printf(TEXT("🔥 SEVERE HEAT - Thirst x%.0f, Movement %d%% Slower"), 
                                         Config->GetHeatSevereThirstMultiplier(), 20);
        NotifColor = FLinearColor(1.0f, 0.5f, 0.0f); // Orange
    }
    else if (BodyTemp >= Config->GetHeatstrokeMildThreshold())
    {
        // MILD: Thirst X2 Faster + %10 Movement Slow (from config)
        TargetEffectTag = FGameplayTag::RequestGameplayTag("Status.Survival.Heatstroke.Mild");
        TargetEffectClass = Config->GetHeatstrokeMildEffectClass();
        TargetSeverity = ESurvivalSeverity::Mild;
        NotificationText = FString::Printf(TEXT("🔥 Getting Hot - Thirst x%.0f, Movement %d%% Slower"), 
                                         Config->GetHeatMildThirstMultiplier(), 10);
        NotifColor = FLinearColor::Yellow;
    }
    // HYPOTHERMIA EFFECTS (matches image requirements)
    else if (BodyTemp <= Config->GetHypothermiaExtremeThreshold())
    {
        // EXTREME: Hunger X4 Faster + %30 Movement Slow (from config)
        TargetEffectTag = FGameplayTag::RequestGameplayTag("Status.Survival.Hypothermia.Extreme");
        TargetEffectClass = Config->GetHypothermiaExtremeEffectClass();
        TargetSeverity = ESurvivalSeverity::Extreme;
        NotificationText = FString::Printf(TEXT("🧊 EXTREME COLD - Hunger x%.0f, Movement %d%% Slower!"), 
                                         Config->GetColdExtremeHungerMultiplier(), 30);
        NotifColor = FLinearColor::Red;
    }
    else if (BodyTemp <= Config->GetHypothermiaHeavyThreshold())
    {
        // HEAVY: Hunger X3 Faster + %20 Movement Slow (from config)
        TargetEffectTag = FGameplayTag::RequestGameplayTag("Status.Survival.Hypothermia.Severe");
        TargetEffectClass = Config->GetHypothermiaSevereEffectClass();
        TargetSeverity = ESurvivalSeverity::Heavy;
        NotificationText = FString::Printf(TEXT("🧊 SEVERE COLD - Hunger x%.0f, Movement %d%% Slower"), 
                                         Config->GetColdSevereHungerMultiplier(), 20);
        NotifColor = FLinearColor(0.5f, 0.8f, 1.0f); // Light Blue
    }
    else if (BodyTemp <= Config->GetHypothermiaMildThreshold())
    {
        // MILD: Hunger X2 Faster + %10 Movement Slow (from config)
        TargetEffectTag = FGameplayTag::RequestGameplayTag("Status.Survival.Hypothermia.Mild");
        TargetEffectClass = Config->GetHypothermiaMildEffectClass();
        TargetSeverity = ESurvivalSeverity::Mild;
        NotificationText = FString::Printf(TEXT("🧊 Getting Cold - Hunger x%.0f, Movement %d%% Slower"), 
                                         Config->GetColdMildHungerMultiplier(), 10);
        NotifColor = FLinearColor::Yellow;
    }

    // SMART STATE MANAGEMENT: Remove incorrect effects, apply correct ones
    TArray<FActiveEffect> CurrentEffects = StatusEffectManagerComponent->GetActiveEffects();
    bool bHasTargetEffect = false;
    
    for (const FActiveEffect& Effect : CurrentEffects)
    {
        if (Effect.Tag.MatchesTag(HeatstrokeParent) || Effect.Tag.MatchesTag(HypothermiaParent))
        {
            if (Effect.Tag == TargetEffectTag)
            {
                bHasTargetEffect = true;
            }
            else
            {
                // Remove incorrect temperature effect
                StatusEffectManagerComponent->Nomad_RemoveStatusEffect(Effect.Tag);
                UE_LOG_SURVIVAL(Log, TEXT("Removed outdated temperature effect: %s"), *Effect.Tag.ToString());
            }
        }
    }
    
    // Apply target effect if not already active and target is valid
    if (TargetEffectTag.IsValid() && TargetEffectClass && !bHasTargetEffect)
    {
        ApplyStatusEffect(TargetEffectClass, TargetSeverity, 0.0f);
        
        // All penalties (thirst/hunger multipliers, movement speed) handled by PersistentAttributeModifier
        BroadcastSurvivalNotification(NotificationText, NotifColor, 5.0f);
        
        UE_LOG_SURVIVAL(Log, TEXT("Applied temperature effect: %s (Severity: %s)"), 
                        *TargetEffectTag.ToString(),
                        *StaticEnum<ESurvivalSeverity>()->GetNameStringByValue((int64)TargetSeverity));
    }
}

void UNomadSurvivalNeedsComponent::ApplyStatusEffect(TSubclassOf<UNomadSurvivalStatusEffect> EffectClass, ESurvivalSeverity Severity, float DoTPercent)
{
    if (!EffectClass || !StatusEffectManagerComponent)
    {
        UE_LOG_SURVIVAL(Warning, TEXT("Cannot apply survival effect - invalid class or manager"));
        return;
    }

    // For survival effects with DoT
    if (DoTPercent > 0.0f)
    {
        UNomadSurvivalStatusEffect* AppliedEffect = 
            StatusEffectManagerComponent->ApplyHazardDoTEffectWithPercent(EffectClass, DoTPercent);
        
        if (AppliedEffect)
        {
            AppliedEffect->SetSeverityLevel(Severity);
        }
    }
    else
    {
        // For effects without DoT (mild effects)
        StatusEffectManagerComponent->ApplyInfiniteStatusEffect(EffectClass);
        
        // Find and set severity
        const UNomadSurvivalStatusEffect* CDO = EffectClass.GetDefaultObject();
        if (CDO)
        {
            FGameplayTag EffectTag = CDO->GetEffectiveTag();
            const int32 Index = StatusEffectManagerComponent->FindActiveEffectIndexByTag(EffectTag);
            if (Index != INDEX_NONE)
            {
                const TArray<FActiveEffect>& ActiveEffects = StatusEffectManagerComponent->GetActiveEffects();
                if (UNomadSurvivalStatusEffect* ActiveEffect = 
                    Cast<UNomadSurvivalStatusEffect>(ActiveEffects[Index].EffectInstance))
                {
                    ActiveEffect->SetSeverityLevel(Severity);
                }
            }
        }
    }
}

void UNomadSurvivalNeedsComponent::RemoveAllSurvivalEffects()
{
    if (!StatusEffectManagerComponent) return;

    // Remove all survival-related status effects by their gameplay tags
    // This ensures clean state transitions when conditions change
    
    // Remove all temperature effects
    if (StatusEffectManagerComponent)
    {
        // Remove all heatstroke effects
        StatusEffectManagerComponent->Nomad_RemoveStatusEffect(
            FGameplayTag::RequestGameplayTag("Status.Survival.Heatstroke.Mild"));
        StatusEffectManagerComponent->Nomad_RemoveStatusEffect(
            FGameplayTag::RequestGameplayTag("Status.Survival.Heatstroke.Severe"));
        StatusEffectManagerComponent->Nomad_RemoveStatusEffect(
            FGameplayTag::RequestGameplayTag("Status.Survival.Heatstroke.Extreme"));
            
        // Remove all hypothermia effects  
        StatusEffectManagerComponent->Nomad_RemoveStatusEffect(
            FGameplayTag::RequestGameplayTag("Status.Survival.Hypothermia.Mild"));
        StatusEffectManagerComponent->Nomad_RemoveStatusEffect(
            FGameplayTag::RequestGameplayTag("Status.Survival.Hypothermia.Severe"));
        StatusEffectManagerComponent->Nomad_RemoveStatusEffect(
            FGameplayTag::RequestGameplayTag("Status.Survival.Hypothermia.Extreme"));
            
        // Remove all starvation effects
        StatusEffectManagerComponent->Nomad_RemoveStatusEffect(
            FGameplayTag::RequestGameplayTag("Status.Survival.Starvation.Mild"));
        StatusEffectManagerComponent->Nomad_RemoveStatusEffect(
            FGameplayTag::RequestGameplayTag("Status.Survival.Starvation.Severe"));
            
        // Remove all dehydration effects
        StatusEffectManagerComponent->Nomad_RemoveStatusEffect(
            FGameplayTag::RequestGameplayTag("Status.Survival.Dehydration.Mild"));
        StatusEffectManagerComponent->Nomad_RemoveStatusEffect(
            FGameplayTag::RequestGameplayTag("Status.Survival.Dehydration.Severe"));
    }
    
    // Ensure movement speed is properly synced after removing all survival effects
    // This ensures any lingering movement modifiers are cleaned up
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter)
    {
        UNomadBaseStatusEffect::SyncMovementSpeedFromStatusEffects(OwnerCharacter);
        UE_LOG_SURVIVAL(Verbose, TEXT("[SURVIVAL] Synced movement speed after removing all survival effects"));
    }
}

// ======== Legacy Status Effect System (Compatibility) ========

void UNomadSurvivalNeedsComponent::ApplyGenericStatusEffect(const TSubclassOf<UNomadBaseStatusEffect>& InStatusEffectClass, float InDoTPercent) const
{
    // Only proceed if both component and effect class are valid
    if (StatusEffectManagerComponent && *InStatusEffectClass)
    {
        // Apply status effect using ACF system
        // Effect should be configured as non-stacking and retriggerable in the asset
        StatusEffectManagerComponent->ApplyHazardDoTEffectWithPercent(
            InStatusEffectClass,
            InDoTPercent
        );
    }
    // Silently fail if components/classes are missing - survival system continues without effects
    // This allows the system to work even if status effect system is not available
}

void UNomadSurvivalNeedsComponent::TryRemoveStatusEffect(const FGameplayTag StatusEffectTag) const
{
    // Only proceed if component is valid and tag is set
    if (StatusEffectManagerComponent && StatusEffectTag.IsValid())
    {
        // Remove status effect by gameplay tag using ACF system
        StatusEffectManagerComponent->Nomad_RemoveStatusEffect(StatusEffectTag);
    }
    // Silently fail if components/tags are missing - prevents crashes from invalid removal attempts
}

// ======== Warning System ========

void UNomadSurvivalNeedsComponent::MaybeFireStarvationWarning(const float CurrentInGameTime, const float CurrentHunger)
{
    // Early exit if config is missing
    if (!GetConfig()) return;
    
    // Check if player is in warning condition (hungry but not starving)
    // Warning threshold is typically higher than critical (0) threshold
    if (CurrentHunger > 0.f && CurrentHunger <= GetConfig()->GetStarvationWarningThreshold())
    {
        // Use escalating warning system to determine if warning should fire
        // Warnings become more frequent over time to build urgency
        if (ShouldFireEscalatingWarning("Starvation", CurrentInGameTime, GetConfig()->GetStarvationWarningCooldown(), StarvationWarningCount))
        {
            // Fire the Blueprint event first for gameplay systems that need raw hunger value
            OnStarvationWarning.Broadcast(CurrentHunger);
            
            // Determine notification details based on escalation level
            // Higher warning counts get more urgent text and colors
            FString NotificationText;
            FLinearColor NotificationColor;
            float DisplayDuration;
            
            switch (StarvationWarningCount)
            {
            case 1:
                // First warning - gentle reminder with friendly tone
                NotificationText = TEXT("Getting Hungry - Find Food Soon!");
                NotificationColor = FLinearColor::Yellow;
                DisplayDuration = 3.0f;
                break;
            case 2:
            case 3:
                // Escalated warning - more urgent tone and color
                NotificationText = TEXT("Still Hungry - Food Needed!");
                NotificationColor = FLinearColor(1.0f, 0.5f, 0.0f); // Orange
                DisplayDuration = 4.0f;
                break;
            default:
                // Critical warning - emergency tone with red color and longer display
                NotificationText = TEXT("CRITICAL HUNGER - EAT NOW!");
                NotificationColor = FLinearColor::Red;
                DisplayDuration = 5.0f;
                break;
            }
            
            // Broadcast notification to all listening systems (UI, audio, effects, etc.)
            BroadcastSurvivalNotification(NotificationText, NotificationColor, DisplayDuration);
            
            // Log warning for debugging and analytics
            UE_LOG_SURVIVAL_EVENTS(Log, TEXT("Starvation Warning #%d - Hunger: %.2f (Time: %.2f)"), 
                                   StarvationWarningCount, CurrentHunger, CurrentInGameTime);
        }
    }
    else
    {
        // Reset warning system when not in warning condition
        // This ensures immediate warning when re-entering condition and resets escalation
        LastStarvationWarningTime = -1.f;
        StarvationWarningCount = 0;
    }
}

void UNomadSurvivalNeedsComponent::MaybeFireDehydrationWarning(const float CurrentInGameTime, const float CurrentThirst)
{
    // Early exit if config is missing
    if (!GetConfig()) return;
    
    // Check if player is in warning condition (thirsty but not dehydrated)
    // Warning threshold is typically higher than critical (0) threshold
    if (CurrentThirst > 0.f && CurrentThirst <= GetConfig()->GetDehydrationWarningThreshold())
    {
        // Use escalating warning system to determine if warning should fire
        // Warnings become more frequent over time to build urgency
        if (ShouldFireEscalatingWarning("Dehydration", CurrentInGameTime, GetConfig()->GetDehydrationWarningCooldown(), DehydrationWarningCount))
        {
            // Fire the Blueprint event first for gameplay systems that need raw thirst value
            OnDehydrationWarning.Broadcast(CurrentThirst);
            
            // Determine notification details based on escalation level
            // Higher warning counts get more urgent text and colors
            FString NotificationText;
            FLinearColor NotificationColor;
            float DisplayDuration;
            
            switch (DehydrationWarningCount)
            {
                case 1:
                    // First warning - gentle reminder with friendly tone
                    NotificationText = TEXT("Getting Thirsty - Find Water Soon!");
                    NotificationColor = FLinearColor::Blue;
                    DisplayDuration = 3.0f;
                    break;
                case 2:
                case 3:
                    // Escalated warning - more urgent tone and lighter blue color
                    NotificationText = TEXT("Still Thirsty - Water Needed!");
                    NotificationColor = FLinearColor(0.0f, 0.7f, 1.0f); // Light Blue
                    DisplayDuration = 4.0f;
                    break;
                default:
                    // Critical warning - emergency tone with red color (overrides blue theme for urgency)
                    NotificationText = TEXT("CRITICAL THIRST - DRINK NOW!");
                    NotificationColor = FLinearColor::Red;
                    DisplayDuration = 5.0f;
                    break;
            }
            
            // Broadcast notification to all listening systems (UI, audio, effects, etc.)
            BroadcastSurvivalNotification(NotificationText, NotificationColor, DisplayDuration);
            
            // Log warning for debugging and analytics
            UE_LOG_SURVIVAL_EVENTS(Log, TEXT("Dehydration Warning #%d - Thirst: %.2f (Time: %.2f)"), 
                                   DehydrationWarningCount, CurrentThirst, CurrentInGameTime);
        }
    }
    else
    {
        // Reset warning system when not in warning condition
        // This ensures immediate warning when re-entering condition and resets escalation
        LastDehydrationWarningTime = -1.f;
        DehydrationWarningCount = 0;
    }
}

void UNomadSurvivalNeedsComponent::MaybeFireHeatstrokeWarning(const float CurrentInGameTime, const float InBodyTemperature)
{
    // Early exit if config is missing
    if (!GetConfig()) return;
    
    // Get threshold values for heatstroke warning zone calculation
    const float HeatstrokeThreshold = GetConfig()->GetHeatstrokeThreshold();
    const float HeatstrokeWarningDelta = GetConfig()->GetHeatstrokeWarningDelta();
    
    // Check if player is in heatstroke warning zone (approaching but not yet at heatstroke)
    if (InBodyTemperature >= HeatstrokeThreshold - HeatstrokeWarningDelta && 
        InBodyTemperature < HeatstrokeThreshold)
    {
        // Use escalating warning system to determine if warning should fire
        // Temperature warnings tend to escalate faster due to immediate danger
        if (ShouldFireEscalatingWarning("Heatstroke", CurrentInGameTime, GetConfig()->GetHeatstrokeWarningCooldown(), HeatstrokeWarningCount))
        {
            // Fire the Blueprint event first for gameplay systems that need raw temperature value
            OnHeatstrokeWarning.Broadcast(InBodyTemperature);
            
            // Determine notification details based on escalation level
            // Temperature warnings escalate faster than hunger/thirst due to immediate danger
            FString NotificationText;
            FLinearColor NotificationColor;
            float DisplayDuration;
            
            switch (HeatstrokeWarningCount)
            {
                case 1:
                    // First warning - immediate concern with orange color (hotter than yellow)
                    NotificationText = TEXT("Overheating - Find Shade!");
                    NotificationColor = FLinearColor(1.0f, 0.5f, 0.0f); // Orange
                    DisplayDuration = 4.0f;
                    break;
                case 2:
                    // Second warning - already escalated due to temperature danger
                    NotificationText = TEXT("Dangerously Hot - Cool Down!");
                    NotificationColor = FLinearColor(1.0f, 0.3f, 0.0f); // Red-Orange
                    DisplayDuration = 5.0f;
                    break;
                default:
                    // Critical warning - maximum urgency with longest display time
                    NotificationText = TEXT("HEATSTROKE IMMINENT - COOL DOWN NOW!");
                    NotificationColor = FLinearColor::Red;
                    DisplayDuration = 6.0f;
                    break;
            }
            
            // Broadcast notification to all listening systems (UI, audio, effects, etc.)
            BroadcastSurvivalNotification(NotificationText, NotificationColor, DisplayDuration);
            
            // Log warning for debugging and analytics
            UE_LOG_SURVIVAL_EVENTS(Log, TEXT("Heatstroke Warning #%d - Body Temp: %.2f (Time: %.2f)"), 
                                   HeatstrokeWarningCount, InBodyTemperature, CurrentInGameTime);
        }
    }
    else
    {
        // Reset warning system when not in warning condition
        // This ensures immediate warning when re-entering condition and resets escalation
        LastHeatstrokeWarningTime = -1.f;
        HeatstrokeWarningCount = 0;
    }
}

void UNomadSurvivalNeedsComponent::MaybeFireHypothermiaWarning(const float CurrentInGameTime, const float InBodyTemperature)
{
    // Early exit if config is missing
    if (!GetConfig()) return;
    
    // Get threshold values for hypothermia warning zone calculation
    const float HypothermiaThreshold = GetConfig()->GetHypothermiaThreshold();
    const float HypothermiaWarningDelta = GetConfig()->GetHypothermiaWarningDelta();
    
    // Check if player is in hypothermia warning zone (approaching but not yet at hypothermia)
    if (InBodyTemperature <= HypothermiaThreshold + HypothermiaWarningDelta && 
        InBodyTemperature > HypothermiaThreshold)
    {
        // Use escalating warning system to determine if warning should fire
        // Temperature warnings tend to escalate faster due to immediate danger
        if (ShouldFireEscalatingWarning("Hypothermia", CurrentInGameTime, GetConfig()->GetHypothermiaWarningCooldown(), HypothermiaWarningCount))
        {
            // Fire the Blueprint event first for gameplay systems that need raw temperature value
            OnHypothermiaWarning.Broadcast(InBodyTemperature);
            
            // Determine notification details based on escalation level
            // Cold warnings use blue color scheme transitioning to red for critical states
            FString NotificationText;
            FLinearColor NotificationColor;
            float DisplayDuration;
            
            switch (HypothermiaWarningCount)
            {
                case 1:
                    // First warning - light blue to indicate cold but not immediate danger
                    NotificationText = TEXT("Getting Cold - Find Warmth!");
                    NotificationColor = FLinearColor(0.5f, 0.8f, 1.0f); // Light Blue
                    DisplayDuration = 4.0f;
                    break;
                case 2:
                    // Second warning - darker blue to indicate increasing danger
                    NotificationText = TEXT("Dangerously Cold - Warm Up!");
                    NotificationColor = FLinearColor(0.2f, 0.5f, 1.0f); // Blue
                    DisplayDuration = 5.0f;
                    break;
                default:
                    // Critical warning - red color overrides blue theme for maximum urgency
                    NotificationText = TEXT("HYPOTHERMIA RISK - GET WARM NOW!");
                    NotificationColor = FLinearColor::Red;
                    DisplayDuration = 6.0f;
                    break;
            }
            
            // Broadcast notification to all listening systems (UI, audio, effects, etc.)
            BroadcastSurvivalNotification(NotificationText, NotificationColor, DisplayDuration);
            
            // Log warning for debugging and analytics
            UE_LOG_SURVIVAL_EVENTS(Log, TEXT("🥶 Hypothermia Warning #%d - Body Temp: %.2f (Time: %.2f)"), 
                                   HypothermiaWarningCount, InBodyTemperature, CurrentInGameTime);
        }
    }
    else
    {
        // Reset warning system when not in warning condition
        // This ensures immediate warning when re-entering condition and resets escalation
        LastHypothermiaWarningTime = -1.f;
        HypothermiaWarningCount = 0;
    }
}

bool UNomadSurvivalNeedsComponent::ShouldFireEscalatingWarning(const FString& WarningType, const float CurrentTime, const float BaseCooldown, int32& WarningCount)
{
    // Use pointer to avoid repeated string comparisons and switch statements
    // This is a minor performance optimization for frequently called function
    float* LastTime = nullptr;
    
    // Map warning type string to corresponding time tracking variable
    if (WarningType == "Starvation")
        LastTime = &LastStarvationWarningTime;
    else if (WarningType == "Dehydration")  
        LastTime = &LastDehydrationWarningTime;
    else if (WarningType == "Heatstroke")
        LastTime = &LastHeatstrokeWarningTime;
    else if (WarningType == "Hypothermia")
        LastTime = &LastHypothermiaWarningTime;
    
    // Early exit if warning type is not recognized
    if (!LastTime) return false;

    // Calculate escalating cooldown - warnings get more frequent over time to build urgency
    float DynamicCooldown = BaseCooldown;
    
    // After 3rd warning, warnings fire twice as frequently
    if (WarningCount >= 3)
    {
        DynamicCooldown *= 0.5f; // 2x faster after 3rd warning
    }
    // After 6th warning, warnings fire four times as frequently (cumulative)
    if (WarningCount >= 6)
    {
        DynamicCooldown *= 0.5f; // 4x faster after 6th warning (0.5 * 0.5 = 0.25 of original)
    }
    
    // Check if enough time has passed since last warning OR if this is the first warning
    if (*LastTime < 0.f || (CurrentTime - *LastTime) >= DynamicCooldown)
    {
        // Update last warning time and increment counter
        *LastTime = CurrentTime;
        WarningCount++;
        return true; // Fire the warning
    }
    
    // Not enough time has passed since last warning
    return false;
}

void UNomadSurvivalNeedsComponent::BroadcastSurvivalNotification(const FString& NotificationText, const FLinearColor& Color, const float Duration) const
{
    // Broadcast to all listening systems (UI widgets, audio managers, screen effect systems, etc.)
    // This unified notification system allows multiple systems to respond to the same survival event
    OnSurvivalNotification.Broadcast(NotificationText, Color, Duration);
    
#if !UE_BUILD_SHIPPING
    // Keep debug output for development builds only
    // This provides immediate visual feedback during development without affecting shipping performance
    if (GEngine)
    {
        // Convert linear color to FColor for engine debug display
        GEngine->AddOnScreenDebugMessage(-1, Duration, Color.ToFColor(true), NotificationText);
    }
#endif
}

// ======== State Check Helper Functions ========

bool UNomadSurvivalNeedsComponent::IsStarving(const float CachedHunger) const
{
    // Early exit if config is missing
    if (!GetConfig()) return false;
    
    // Starvation occurs when hunger reaches or drops below 0
    // This is the most severe hunger state that triggers debuff effects
    return CachedHunger <= 0.f;
}

bool UNomadSurvivalNeedsComponent::IsHungry(const float CachedHunger) const
{
    // Early exit if config is missing
    if (!GetConfig()) return false;
    
    // Hungry state is when hunger is low but not yet at starvation level
    // This triggers warnings but not debuff effects
    return CachedHunger > 0.f && CachedHunger <= GetConfig()->GetStarvationWarningThreshold();
}

bool UNomadSurvivalNeedsComponent::IsDehydrated(const float CachedThirst) const
{
    // Early exit if config is missing
    if (!GetConfig()) return false;
    
    // Dehydration occurs when thirst reaches or drops below 0
    // This is the most severe thirst state that triggers debuff effects
    return CachedThirst <= 0.f;
}

bool UNomadSurvivalNeedsComponent::IsThirsty(const float CachedThirst) const
{
    // Early exit if config is missing
    if (!GetConfig()) return false;
    
    // Thirsty state is when thirst is low but not yet at dehydration level
    // This triggers warnings but not debuff effects
    return CachedThirst > 0.f && CachedThirst <= GetConfig()->GetDehydrationWarningThreshold();
}

bool UNomadSurvivalNeedsComponent::IsHeatstroke(const float CachedBodyTemp) const
{
    // Early exit if config is missing
    if (!GetConfig()) return false;
    
    // Heatstroke occurs when body temperature reaches or exceeds the heatstroke threshold
    // This triggers immediate hazard state that can cause health damage
    return CachedBodyTemp >= GetConfig()->GetHeatstrokeThreshold();
}

bool UNomadSurvivalNeedsComponent::IsHypothermic(const float CachedBodyTemp) const
{
    // Early exit if config is missing
    if (!GetConfig()) return false;
    
    // Hypothermia occurs when body temperature reaches or drops below the hypothermia threshold
    // This triggers immediate hazard state that can cause movement penalties and health issues
    return CachedBodyTemp <= GetConfig()->GetHypothermiaThreshold();
}