// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PickupItemActorData.h"
#include "Core/Data/Item/BaseItemData.h"
#include "GatherableActorData.generated.h"


class ABaseGatherableActor;

/**
 * FGatheredItem
 *
 * Represents a single loot entry that can be spawned or granted when a gatherable resource is harvested.
 */
USTRUCT(BlueprintType)
struct FGatheredItem {
    GENERATED_BODY()

    /**
     * Default constructor initializes pointers to null and leaves ResourceItem default-constructed.
     */
    FGatheredItem()
        : ResourceItem()
          , PickupItemActorData(nullptr) {}

    /**
     * The base data describing the resource item (e.g., stone, wood).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
    FBaseItem ResourceItem;

    /**
     * Localized display name for the gathered item (shown in UI/tooltips).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
    FText GatheredItemName;

    /**
     * Optional actor data used to spawn a pickup actor representing the item in the world.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
    UPickupItemActorData* PickupItemActorData;

    /**
     * Get the display name of this gathered item.
     * @return FText containing the localized item name.
     */
    FORCEINLINE FText GetGatheredItemName() const
    {
        return GatheredItemName;
    }

    /**
     * Get the actor data to use for spawning the pickup actor in the world.
     * @return Pointer to UPickupItemActorData or nullptr if none.
     */
    FORCEINLINE UPickupItemActorData* GetPickupItemActorData() const
    {
        return PickupItemActorData;
    }
};


/**
 * FGatherableActorInfo
 *
 * Encapsulates all configuration options for a gatherable actor type,
 * including visual representation, loot, health, and behavior flags.
 */
USTRUCT(BlueprintType)
struct FGatherableActorInfo
{
    GENERATED_BODY()

    /**
     * Default constructor sets sensible defaults for all gatherable actor properties.
     */
    FGatherableActorInfo()
        : bIsPickupItem(false)
          , bUseNextStage(false)
          , GatherableActorMesh(nullptr)
          , GatheredActorMesh(nullptr)
          , GatherableActorTag(FGameplayTag())
          , CollectResourceTag(FGameplayTag())
          , RequiredToolTag(FGameplayTag())
          , NextStageGatherableActor(nullptr)
          , ItemsToGive()
          , GatherableActorHealth(100)
          , DamagePerHit(25)
          , bUsePhysicsDrop(false)
          , bShouldSpawnedOnTheGround(false)
          , bDestroyAfterGathering(true) {}

    /**
     * Indicates if this actor should be treated purely as a pickup item (no gathering stages).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Option")
    bool bIsPickupItem;

    /**
     * If true, upon gathering this actor will spawn a new actor representing the next stage
     * (e.g., chopping a tree spawns a log actor).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bIsPickupItem"), Category = "Option")
    bool bUseNextStage;

    // === Visual & Mesh Settings ===

    /**
     * Mesh asset to represent the gatherable actor in its initial state (e.g., a full tree).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bIsPickupItem"), Category = "Visual")
    TObjectPtr<UStaticMesh> GatherableActorMesh;

    /**
     * Mesh asset to represent this actor after it has been gathered (used only if bUseNextStage is true).
     * Typically a depleted or partially gathered version.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseNextStage"), Category = "Visual")
    TObjectPtr<UStaticMesh> GatheredActorMesh;

    /**
     * Additional mesh variants for intermediate gather stages (optional).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TArray<UStaticMesh*> GatherStageMeshes;

    // === Tags & Gameplay Identity ===

    /**
     * Gameplay tag to categorize this resource type (e.g., Resource.Tree).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
    FGameplayTag GatherableActorTag;

    /**
     * Gameplay tag to broadcast when the player collects this resource, for triggering UI or effects.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
    FGameplayTag CollectResourceTag;

    /**
     * Tag specifying which tool is required to gather (e.g., Tool.Axe).
     * If left empty, the resource can be gathered by hand.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bIsPickupItem"), Category = "Tags")
    FGameplayTag RequiredToolTag;

    // === Next-Stage Gatherable Support ===

    /**
     * Class to spawn when this actor is fully gathered, representing the next stage (log, rock chunk, etc.).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseNextStage && !bIsPickupItem"), Category = "Chained Gathering")
    TSubclassOf<ABaseGatherableActor> NextStageGatherableActor;

    /**
     * Impulse multiplier applied to the spawned next-stage actor for physics-based ejection.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseNextStage && !bIsPickupItem"), Category = "Chained Gathering")
    float ImpulseMultiplier = 250.f;

    /**
     * Socket name on the original actor where the impulse force should be applied.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseNextStage && !bIsPickupItem"), Category = "Chained Gathering")
    FName ImpulseApplicationSocketName = FName("ForceLocation");

    // === Gathering Outcome ===

    /**
     * List of FGatheredItem entries to give/spawn when gathering completes.
     * Only used if UE does not chain to a next-stage actor.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUseNextStage"), Category = "Loot")
    TArray<FGatheredItem> ItemsToGive;

    // === Health & Damage Handling ===

    /**
     * Hit points of the resource before it breaks and gathering finishes.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bIsPickupItem"), Category = "Health")
    int32 GatherableActorHealth;

    /**
     * Damage applied per gather action/hit. Multiple hits reduce health until zero.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bIsPickupItem"), Category = "Health")
    int32 DamagePerHit;

    // === Drop & Cleanup Settings ===

    /**
     * If true, items dropped or spawned will use physics simulation (bounce, scatter).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    bool bUsePhysicsDrop;

    /**
     * If true, dropped items will simply spawn on the ground without physics applied.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUsePhysicsDrop"), Category = "Behavior")
    bool bShouldSpawnedOnTheGround;

    /**
     * Should the gatherable actor destroy itself after completing gathering?
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
    bool bDestroyAfterGathering;

    // ———— C++ Inline Getters ————

    /** @return true if this actor is configured as a standalone pickup item. */
    FORCEINLINE bool IsPickupItem() const
    {
        return bIsPickupItem;
    }

    /** @return pointer to the initial gatherable mesh. */
    FORCEINLINE UStaticMesh* GetGatherableMesh() const
    {
        return GatherableActorMesh.Get();
    }

    /** @return pointer to the mesh used after initial gathering stage. */
    FORCEINLINE UStaticMesh* GetGatheredMesh() const
    {
        return GatheredActorMesh.Get();
    }

    /** @return reference array of intermediate stage meshes. */
    FORCEINLINE const TArray<UStaticMesh*>& GetGatherStageMeshes() const
    {
        return GatherStageMeshes;
    }

    /** @return gameplay tag identifying the resource type. */
    FORCEINLINE FGameplayTag GetResourceTag() const
    {
        return GatherableActorTag;
    }

    /** @return tag broadcasted when resource is collected. */
    FORCEINLINE FGameplayTag GetCollectTag() const
    {
        return CollectResourceTag;
    }

    /** @return tag representing any required tool to gather this resource. */
    FORCEINLINE FGameplayTag GetRequiredToolTag() const
    {
        return RequiredToolTag;
    }

    /** @return true if chaining to a next-stage actor. */
    FORCEINLINE bool UsesNextStage() const
    {
        return bUseNextStage;
    }

    /** @return class type of the next-stage gatherable actor. */
    FORCEINLINE TSubclassOf<ABaseGatherableActor> GetNextStageClass() const
    {
        return NextStageGatherableActor;
    }

    /** @return impulse force multiplier for next-stage spawn. */
    FORCEINLINE float GetImpulseMultiplier() const
    {
        return ImpulseMultiplier;
    }

    /** @return name of the socket to apply impulse when spawning next stage. */
    FORCEINLINE FName GetImpulseSocketName() const
    {
        return ImpulseApplicationSocketName;
    }

    /** @return constant reference to loot items to give upon gathering. */
    FORCEINLINE const TArray<FGatheredItem>& GetLootItems() const
    {
        return ItemsToGive;
    }

    /** @return maximum hit points of the gatherable actor. */
    FORCEINLINE int32 GetMaxHealth() const
    {
        return GatherableActorHealth;
    }

    /** @return damage applied per gather hit. */
    FORCEINLINE int32 GetDamagePerHit() const
    {
        return DamagePerHit;
    }

    /** @return true if physics-based drops are enabled. */
    FORCEINLINE bool UsesPhysicsDrop() const
    {
        return bUsePhysicsDrop;
    }

    /** @return true if dropped items should spawn directly on ground. */
    FORCEINLINE bool ShouldSpawnOnGround() const
    {
        return bShouldSpawnedOnTheGround;
    }

    /** @return true if actor self-destructs after gathering. */
    FORCEINLINE bool ShouldDestroyAfterGather() const
    {
        return bDestroyAfterGathering;
    }
};


/**
 * UGatherableActorData
 *
 * DataAsset wrapper to expose FGatherableActorInfo configuration to content designers.
 */
UCLASS()
class NOMADDEV_API UGatherableActorData : public UDataAsset {
    GENERATED_BODY()

public:
    /** Designer-facing property grouping all gatherable actor settings. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Actor Information", meta = (ShowOnlyInnerProperties))
    FGatherableActorInfo GatherableActorInfo;

    // ———— Blueprint-Callable Getters ————

    /** @return whether this data asset represents a pickup item actor. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    bool GetIsPickupItem() const
    {
        return GatherableActorInfo.IsPickupItem();
    }

    /** @return static mesh for the initial gatherable state. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    UStaticMesh* GetGatherableMesh() const
    {
        return GatherableActorInfo.GetGatherableMesh();
    }

    /** @return static mesh for the post-gather stage. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    UStaticMesh* GetGatheredMesh() const
    {
        return GatherableActorInfo.GetGatheredMesh();
    }

    /** @return array of intermediate gather stage meshes. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    const TArray<UStaticMesh*>& GetGatherStageMeshes() const
    {
        return GatherableActorInfo.GetGatherStageMeshes();
    }

    /** @return resource gameplay tag. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    FGameplayTag GetResourceTag() const
    {
        return GatherableActorInfo.GetResourceTag();
    }

    /** @return collect resource gameplay tag. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    FGameplayTag GetCollectTag() const
    {
        return GatherableActorInfo.GetCollectTag();
    }

    /** @return required tool tag. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    FGameplayTag GetRequiredToolTag() const
    {
        return GatherableActorInfo.GetRequiredToolTag();
    }

    /** @return whether next-stage chaining is used. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    bool UsesNextStage() const
    {
        return GatherableActorInfo.UsesNextStage();
    }

    /** @return next-stage actor class. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    TSubclassOf<ABaseGatherableActor> GetNextStageClass() const
    {
        return GatherableActorInfo.GetNextStageClass();
    }

    /** @return impulse multiplier for next-stage spawn. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    float GetImpulseMultiplier() const
    {
        return GatherableActorInfo.GetImpulseMultiplier();
    }

    /** @return socket name for impulse application. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    FName GetImpulseSocketName() const
    {
        return GatherableActorInfo.GetImpulseSocketName();
    }

    /** @return list of loot items to grant. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    TArray<FGatheredItem> GetLootItems() const
    {
        return GatherableActorInfo.GetLootItems();
    }

    /** @return maximum health of the gatherable resource. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    int32 GetMaxHealth() const
    {
        return GatherableActorInfo.GetMaxHealth();
    }

    /** @return damage per gather interaction. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    int32 GetDamagePerHit() const
    {
        return GatherableActorInfo.GetDamagePerHit();
    }

    /** @return whether physics-based drops are enabled. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    bool UsesPhysicsDrop() const
    {
        return GatherableActorInfo.UsesPhysicsDrop();
    }

    /** @return whether spawned items should appear directly on the ground. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    bool ShouldSpawnOnGround() const
    {
        return GatherableActorInfo.ShouldSpawnOnGround();
    }

    /** @return whether the gatherable actor should destroy itself after gathering. */
    UFUNCTION(BlueprintPure, Category = "Gatherable Data")
    bool ShouldDestroyAfterGather() const
    {
        return GatherableActorInfo.ShouldDestroyAfterGather();
    }
};