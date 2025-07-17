// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARSStatisticsComponent.h"
#include "ACFActionTypes.h"
#include "Components/ACFEquipmentComponent.h"
#include "Game/ACFDamageCalculation.h"
#include "AIData.generated.h"

UENUM(BlueprintType)
enum class EFeets : uint8
{
    LeftFoot,
    RightFoot, 
    LeftFoot2, 
    RightFoot2 
};

// -----------------------------------------------------------------------------
// Struct for Statistics Component related settings
// -----------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FStatisticsCompData
{
    GENERATED_BODY();

    /** Default constructor initializing all members with safe defaults */
    FStatisticsCompData()
        : bAutoInitialize(false)
        , StatsLoadMethod(EStatsLoadMethod::EGenerateFromDefaultsPrimary)
        , DefaultAttributeSet()
        , LevelingType(ELevelingType::ECantLevelUp)
        , AttributesByLevelConfig(nullptr)
        , CharacterLevel(1)
        , ExpForNextLevelCurve(nullptr)
        , ExpToGiveOnDeath(0.f)
        , PerksObtainedOnLevelUp(1)
        , ExpToGiveOnDeathByCurrentLevel(nullptr)
        , StatisticConsumptionMultiplier()
        , bCanRegenerateStatistics(true)
        , RegenerationTimeInterval(0.2f)
    {}

    /** If true, InitializeAttributeSet is called automatically on BeginPlay server-side */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    bool bAutoInitialize;

    /** How statistics and attributes are generated */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EStatsLoadMethod StatsLoadMethod;

    /** Default attribute set for generation methods */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, meta=(EditCondition="StatsLoadMethod!=EStatsLoadMethod::ELoadByLevel"))
    FAttributesSet DefaultAttributeSet;

    /** Leveling type controlling stat growth */
    UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadWrite)
    ELevelingType LevelingType;

    /** Curve asset for level-based attribute scaling */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="LevelingType==ELevelingType::EGenerateNewStatsFromCurves&&StatsLoadMethod==EStatsLoadMethod::ELoadByLevel"))
    UARSLevelingSystemDataAsset* AttributesByLevelConfig;

    /** Character level for stat generation */
    UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadWrite)
    int32 CharacterLevel;

    /** Curve defining exp required per level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="LevelingType!=ELevelingType::ECantLevelUp"))
    UCurveFloat* ExpForNextLevelCurve;

    /** Exp granted to others on death if this character cannot level up */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="LevelingType==ELevelingType::ECantLevelUp"))
    float ExpToGiveOnDeath;

    /** Perks gained per level up (manual assignment) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="LevelingType==ELevelingType::EAssignPerksManually"))
    int32 PerksObtainedOnLevelUp;

    /** Curve defining exp dropped on death by current level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="LevelingType!=ELevelingType::ECantLevelUp"))
    UCurveFloat* ExpToGiveOnDeathByCurrentLevel;

    /** Multiplier for statistic consumption events */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FGameplayTag, float> StatisticConsumptionMultiplier;

    /** If true, statistics regenerate over time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanRegenerateStatistics;

    /** Time interval between regeneration ticks (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RegenerationTimeInterval;
};

// -----------------------------------------------------------------------------
// Struct for Actions Manager related settings
// -----------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FActionsManagerData
{
    GENERATED_BODY();

    /** Default constructor initializing all members */
    FActionsManagerData()
        : bCanTick(true)
        , bPrintDebugInfo(false)
        , ActionsSet(nullptr)
        , MovesetActions()
    {}

    /** Enables ticking for the actions component */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
    bool bCanTick;

    /** Enables debug info display for actions */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
    bool bPrintDebugInfo;

    /** Default set of actions for this character */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    TSubclassOf<UACFActionsSet> ActionsSet;

    /** Specific moveset-based action overrides */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(TitleProperty="TagName"))
    TArray<FActionsSet> MovesetActions;
};

// -----------------------------------------------------------------------------
// Struct for Damage Handler related settings
// -----------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FDamageHandlerData
{
    GENERATED_BODY();

    /** Default constructor initializing all members */
    FDamageHandlerData()
        : bUseBlockingCollisionChannel(false)
        , DamageCalculatorClass(nullptr)
        , HitResponseActions()
    {}

    /** Use blocking collision channel for damage checks */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseBlockingCollisionChannel;

    /** Class used for calculating damage on hit */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UACFDamageCalculation> DamageCalculatorClass;

    /** Actions triggered when hit (dodge, parry, etc.) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FOnHitActionChances> HitResponseActions;
};

// -----------------------------------------------------------------------------
// Struct for Equipment Component related settings
// -----------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FEquipmentCompData
{
    GENERATED_BODY();

    /** Default constructor initializing all members */
    FEquipmentCompData()
        : AvailableEquipmentSlot()
        , AllowedWeaponTypes()
        , bDestroyItemsOnDeath(true)
        , bDropItemsOnDeath(true)
        , bCollapseDropInASingleWorldItem(true)
        , bUpdateMainMeshVisibility(true)
        , MaxInventorySlots(40)
        , bAutoEquipItem(true)
        , MaxInventoryWeight(180.f)
        , StartingItems()
    {}

    /** Tags for equipment slots available */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FGameplayTag> AvailableEquipmentSlot;

    /** Weapon types allowed for this character */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FGameplayTag> AllowedWeaponTypes;

    /** Destroy equipped items on character death */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
    bool bDestroyItemsOnDeath;

    /** Drop inventory items on death */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
    bool bDropItemsOnDeath;

    /** Collapse drops into a single world item */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
    bool bCollapseDropInASingleWorldItem;

    /** Update main mesh visibility when equipping certain armor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    bool bUpdateMainMeshVisibility;

    /** Maximum inventory slot count */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 MaxInventorySlots;

    /** Auto-equip items picked up from world */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    bool bAutoEquipItem;

    /** Maximum total inventory weight */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    float MaxInventoryWeight;

    /** Starting inventory items */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty="ItemClass"), Category = "Inventory")
    TArray<FStartingItem> StartingItems;
};

// -----------------------------------------------------------------------------
// Struct for Effects Manager related settings
// -----------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FEffectsManagerData
{
    GENERATED_BODY();

    /** Default constructor initializing all members */
    FEffectsManagerData()
        : CharacterEffectsConfig(nullptr)
        , DefaultHitBoneName(TEXT("pelvis"))
        , TraceLengthByActorLocation(200.f)
        , FootstepNoiseByLocomotionState()
        , FootstepNoiseByLocomotionStateWhenCrouched()
        , Duration(0.2f)
        , Material(nullptr)
        , ApplyHitMaterial(true)
        , DamageWidget(nullptr)
        , FeetBoneNames()
        , OnHitRumble(nullptr)
        , EffectsByFoot()
        , Shake(nullptr)
        , MinPos(-20.f)
        , MaxPos(50.f)
    {}

    /** Configuration asset for character effects */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
    class UACFEffectsConfigDataAsset* CharacterEffectsConfig;

    /** Default bone for hits */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
    FName DefaultHitBoneName;

    /** Footstep trace length */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Footstep")
    float TraceLengthByActorLocation;

    /** Noise emitted per locomotion state */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Footstep")
    TMap<ELocomotionState, float> FootstepNoiseByLocomotionState;

    /** Noise emitted when crouched */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Footstep")
    TMap<ELocomotionState, float> FootstepNoiseByLocomotionStateWhenCrouched;

    /** Duration for hit material application */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "HitMaterial")
    float Duration;

    /** Material to apply on hit */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "HitMaterial")
    UMaterialInterface* Material;

    /** Apply hit material or not */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "HitMaterial")
    bool ApplyHitMaterial;

    /** Class of damage widget for UI */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
    TSubclassOf<class UACFDamageWidget> DamageWidget;

    /** Mapping foot to bone names */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
    TMap<EFeets, FName> FeetBoneNames;

    /** Feedback effect for hits */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
    UForceFeedbackEffect* OnHitRumble;

    /** Impact effects per foot */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
    TMap<EFeets, FImpactEffect> EffectsByFoot;

    /** Camera shake on hit */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
    TSubclassOf<class UCameraShakeBase> Shake;

    /** Minimum position offset */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
    float MinPos;

    /** Maximum position offset */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
    float MaxPos;
};

/**
 * Data Asset grouping all AI-related configuration.
 */
UCLASS(BlueprintType)
class NOMADDEV_API UAIData : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Statistics component settings */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    FStatisticsCompData StatisticsComp;

    /** Actions manager settings */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    FActionsManagerData ActionsManager;

    /** Damage handler settings */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    FDamageHandlerData DamageHandler;

    /** Equipment component settings */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    FEquipmentCompData EquipmentComp;

    /** Effects manager settings */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    FEffectsManagerData EffectsManager;
};
