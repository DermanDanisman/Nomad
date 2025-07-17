#pragma once

#include "ACFVendorComponent.h" // Base class for vendor related functionality that we extend.
#include "Components/ACFEquipmentComponent.h" // To access pawn equipment and inventory.
#include "Components/ActorComponent.h" // Basic actor component features.
#include "CoreMinimal.h" // Core engine types like FString, TArray, etc.
#include "ACFCraftRecipeDataAsset.h" // Craft recipe data asset class.
#include "Items/ACFItem.h" // Base item class for items in game.

#include "ACFCraftingComponent.generated.h" // Unreal reflection system header for this class.

class AACFCharacter; // Forward declare player character class.
class UCraftingStationData; // Forward declare crafting station data class.

// Delegate that broadcasts a float progress value from 0.0 to 1.0
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftProgressUpdate, float, Progress);
// Delegate that broadcasts when crafting is completed (no parameters)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftComplete);
// Delegate that broadcasts when crafting is canceled (no parameters)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftCanceled);

// Forward declaration for craft recipe data asset class
class UACFCraftRecipeDataAsset;

/**
 * UACFCraftingComponent
 * Handles crafting and upgrading items.
 * Extends UACFVendorComponent to leverage vendor functionality.
 */
UCLASS(Blueprintable, ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class CRAFTINGSYSTEM_API UACFCraftingComponent : public UACFVendorComponent
{
    GENERATED_BODY()

public:
    // Constructor initializes defaults for this component.
    UACFCraftingComponent();

    /*------------------- CHECKS -----------------------------------*/
    
    /**
     * Checks if a pawn can upgrade the specified item.
     * @param itemToUpgrade - The item to upgrade.
     * @param pawnOwner - The pawn who owns the item.
     * @return true if upgrade possible, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    bool CanPawnUpgradeItem(const FInventoryItem& itemToUpgrade, const APawn* pawnOwner) const;

    /**
     * Checks if a pawn can craft the specified item.
     * @param itemToCraft - Crafting recipe to check.
     * @param buyer - The pawn who wants to craft.
     * @return true if crafting possible, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    bool CanPawnCraftItem(const FACFCraftingRecipe& itemToCraft, const APawn* buyer) const;

    /*------------------- SERVER SIDE -----------------------------------*/

    /**
     * Server-side function to craft an item using a recipe.
     * @param ItemToCraft - Recipe for the item to craft.
     * @param instigator - Pawn performing crafting.
     */
    UFUNCTION(BlueprintCallable, Category = "ACF | Crafting")
    void CraftItem(const FACFCraftingRecipe& ItemToCraft, APawn* instigator);

    /**
     * Server-side function to upgrade an item.
     * @param itemToUpgrade - The item to upgrade.
     * @param instigator - Pawn performing upgrade.
     */
    UFUNCTION(BlueprintCallable, Category = "ACF | Crafting")
    void UpgradeItem(const FInventoryItem& itemToUpgrade, APawn* instigator);

    /*-------------------PLAYER STUFF-----------------------------------*/

    /**
     * Returns a list of all upgradeable items from a pawn's inventory.
     * @param pawn - Pawn whose inventory to check.
     * @return Array of upgradable inventory items.
     */
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    TArray<FInventoryItem> GetAllPawnUpgradableItems(const APawn* pawn) const;

    /**
     * Returns all craftable recipes available in this component.
     * @return Array of craftable recipes.
     */
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    TArray<FACFCraftingRecipe> GetCraftableRecipes() const
    {
        return CraftableItems;
    }

    /**
     * Attempts to find a crafting recipe matching the given base item.
     * @param recipe - Base item to match.
     * @param outRecipe - Out param to receive the matched recipe.
     * @return true if recipe found, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    bool TryGetCraftableRecipeForItem(const FBaseItem& recipe, FACFCraftingRecipe& outRecipe) const;

    /**
     * Adds a new crafting recipe to the list of craftable items.
     * @param recipe - The recipe to add.
     */
    UFUNCTION(BlueprintCallable, Category = "ACF | Setters")
    void AddNewRecipe(const FACFCraftingRecipe& recipe)
    {
        CraftableItems.Add(recipe);
    }

    /**
     * Nomad Dev Team
     * Starts crafting the specified recipe count times, for the instigating character,
     * sending crafted items into the provided target storage.
     *
     * @param Recipe - Crafting recipe to start crafting.
     * @param Count - Number of items to craft.
     * @param InstigatorCharacter - Character instigating the crafting.
     * @param TargetStorage - Storage component to send crafted items to (can be nullptr).
     */
    UFUNCTION(BlueprintCallable, Category = "Crafting")
    void StartCrafting(const FACFCraftingRecipe& Recipe, int32 Count, AACFCharacter* InstigatorCharacter, UACFStorageComponent* TargetStorage);

    /** Cancel the current crafting process */
    UFUNCTION(BlueprintCallable, Category = "Crafting")
    void CancelCrafting();

    // Function called every tick of the crafting timer to update progress and complete crafts.
    void CraftTick();

    // Returns true if currently crafting.
    UFUNCTION(BlueprintCallable, Category = "Crafting")
    bool IsCrafting() { return bIsCrating; };

    /**
     * Nomad Dev Team
     * Calculates max craftable amount of given recipe based on pawn inventory.
     * @param Recipe - Recipe to calculate max craftable count for.
     * @param Pawn - Pawn whose inventory is considered.
     * @return Max number of times the recipe can be crafted.
     */
    UFUNCTION(BlueprintCallable, Category = "ACF | Crafting")
    int32 GetMaxCraftableAmount(const FACFCraftingRecipe& Recipe, const APawn* Pawn) const;

    // Nomad Dev Team - Delegate to broadcast progress updates to Blueprint/UI
    UPROPERTY(BlueprintAssignable, Category = "Crafting | Delegates")
    FOnCraftProgressUpdate OnCraftProgressUpdate;

    // Nomad Dev Team - Delegate to broadcast when crafting completes
    UPROPERTY(BlueprintAssignable, Category = "Crafting | Delegates")
    FOnCraftComplete OnCraftComplete;

    // Nomad Dev Team - Delegate to broadcast when crafting cancels
    UPROPERTY(BlueprintAssignable, Category = "Crafting | Delegates")
    FOnCraftCanceled OnCraftCanceled;

protected:
    // Called once when the game starts or component initialized.
    virtual void BeginPlay() override;

    // List of crafting recipe data assets editable in editor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    TArray<UACFCraftRecipeDataAsset*> ItemsRecipes;

    // Array holding all crafting recipes available at runtime.
    UPROPERTY()
    TArray<FACFCraftingRecipe> CraftableItems;

private:
    /**
     * Nomad Dev Team
     */
    int32 RemainingCraftCount = 0;          // Number of crafts left to process
    float CurrentCraftProgress = 0.f;       // Current progress [0..1]
    bool bIsCrating = false;                 // Flag: Is crafting active
    FACFCraftingRecipe CurrentRecipe;       // Recipe currently being crafted
    FTimerHandle CraftTimerHandle;          // Timer handle for CraftTick callback
    UPROPERTY()
    TObjectPtr<AACFCharacter> CraftInstigator = nullptr; // Pointer to instigator character

    // Nomad Dev Team: Store target storage component weak pointer safely to avoid dangling
    TWeakObjectPtr<UACFStorageComponent> CurrentTargetStorage;
    /**
     * Nomad Dev Team
     */
};