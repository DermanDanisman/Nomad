#include "ACFCraftingComponent.h"
#include "ACFItemsManagerComponent.h"
#include "Components/ACFEquipmentComponent.h"
#include "ACFCraftRecipeDataAsset.h"
#include "Actors/ACFCharacter.h"

// Constructor disables ticking by default
UACFCraftingComponent::UACFCraftingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// On begin play, populate CraftableItems from editable data assets
void UACFCraftingComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!ItemsRecipes.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Crafting] No recipe assets assigned"));
    }

    for (UACFCraftRecipeDataAsset* RecipeAsset : ItemsRecipes)
    {
        if (RecipeAsset)
        {
            AddNewRecipe(RecipeAsset->GetCraftingRecipe());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[Crafting] Null recipe asset in ItemsRecipes array"));
        }
    }
}

/*------------------- CHECKS -----------------------------------*/

bool UACFCraftingComponent::CanPawnUpgradeItem(const FInventoryItem& itemToUpgrade, const APawn* pawnOwner) const
{
    if (!pawnOwner)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Crafting] CanPawnUpgradeItem called with null pawn"));
        return false;
    }

    const auto& Inventory = GetPawnInventory(pawnOwner);
    if (!Inventory.Contains(itemToUpgrade))
    {
        return false;
    }

    // Check if pawn has equipment component to verify requirements
    UACFEquipmentComponent* equipComp = GetPawnEquipment(pawnOwner);
    
    if (!equipComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Crafting] No EquipmentComponent on pawn %s"), *pawnOwner->GetName());
        return false;
    }

    if (!itemToUpgrade.ItemInfo.bUpgradable)
    {
        return false;
    }
    
    const int32 Currency = GetPawnCurrency(pawnOwner);
    const int32 Cost     = PriceMultiplierOnSell * itemToUpgrade.ItemInfo.UpgradeCurrencyCost;
    if (Currency < Cost)
    {
        return false;
    }

    return equipComp->HasEnoughItemsOfType(itemToUpgrade.ItemInfo.RequiredItemsToUpgrade);
}

bool UACFCraftingComponent::CanPawnCraftItem(const FACFCraftingRecipe& itemToCraft, const APawn* buyer) const
{
    if (!buyer)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Crafting] CanPawnCraftItem called with null pawn"));
        return false;
    }

    UACFEquipmentComponent* EquipComp = GetPawnEquipment(buyer);
    if (!EquipComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Crafting] No EquipmentComponent on pawn %s"), *buyer->GetName());
        return false;
    }

    // Check inventory space
    const int32 FreeSlots = EquipComp->NumberOfItemCanTake(itemToCraft.OutputItem.ItemClass);
    if (FreeSlots < itemToCraft.OutputItem.Count)
    {
        return false;
    }

    // Check resource items & currency
    const bool bHasResources = EquipComp->HasEnoughItemsOfType(itemToCraft.RequiredItems);
    const int32 Currency     = GetPawnCurrency(buyer);
    const int32 Cost         = PriceMultiplierOnSell * itemToCraft.CraftingCost;

    return bHasResources && (Currency >= Cost);
}

/* ----------- TO SERVER---------------*/

void UACFCraftingComponent::CraftItem(const FACFCraftingRecipe& ItemToCraft, APawn* instigator)
{
    if (!instigator)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Crafting] CraftItem called with null instigator"));
        return;
    }

    UACFItemsManagerComponent* Manager = GetItemsManager();
    if (!Manager)
    {
        UE_LOG(LogTemp, Error, TEXT("[Crafting] No ItemsManagerComponent found"));
        return;
    }

    Manager->CraftItem(ItemToCraft, instigator, this, nullptr);
}

void UACFCraftingComponent::UpgradeItem(const FInventoryItem& itemToUpgrade, APawn* instigator)
{
    if (!instigator)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Crafting] UpgradeItem called with null instigator"));
        return;
    }

    UACFItemsManagerComponent* Manager = GetItemsManager();
    if (!Manager)
    {
        UE_LOG(LogTemp, Error, TEXT("[Crafting] No ItemsManagerComponent found"));
        return;
    }

    Manager->UpgradeItem(itemToUpgrade, instigator, this);
}

/*-------------------PLAYER STUFF-----------------------------------*/

TArray<FInventoryItem> UACFCraftingComponent::GetAllPawnUpgradableItems(const APawn* pawn) const
{
    TArray<FInventoryItem> Result;
    if (!pawn) return Result;

    const auto& Inventory = GetPawnInventory(pawn);
    for (const FInventoryItem& Item : Inventory)
    {
        if (Item.ItemInfo.bUpgradable)
        {
            Result.Add(Item);
        }
    }
    return Result;
}

bool UACFCraftingComponent::TryGetCraftableRecipeForItem(const FBaseItem& recipe, FACFCraftingRecipe& outRecipe) const
{
    if (CraftableItems.Contains(recipe)) {
        outRecipe = *CraftableItems.FindByKey(recipe);
        return true;
    }
    return false;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

/**
 * Nomad Dev Team
 * Calculates the maximum number of times the given recipe can be crafted
 * based on the pawn’s current inventory counts.
 */
int32 UACFCraftingComponent::GetMaxCraftableAmount(const FACFCraftingRecipe& Recipe, const APawn* Pawn) const
{
    if (!Pawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Crafting] GetMaxCraftableAmount called with null pawn"));
        return 0;
    }
    
    // Retrieve the pawn’s equipment component to access their inventory.
    UACFEquipmentComponent* EquipComp = GetPawnEquipment(Pawn);
    if (!EquipComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Crafting] No EquipmentComponent on pawn %s"), *Pawn->GetName());
        return 0;
    }

    // Start with an “infinite” upper bound; we’ll take the minimum across all requirements.
    int32 MaxCraftable = TNumericLimits<int32>::Max();

    // Loop through each item required by the recipe.
    for (const FBaseItem& Required : Recipe.RequiredItems)
    {
        if (!Required.ItemClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Crafting] Recipe has null Required.ItemClass"));
            return 0;
        }
        
        // Count how many of this class the pawn currently has in total.
        int32 AvailableCount = EquipComp->GetTotalCountOfItemsByClass(Required.ItemClass);

        // Determine how many crafts this one resource alone would permit.
        // If Required.Count is zero (shouldn’t happen), we default to 0 to avoid division by zero.
        int32 ThisItemAllows = (Required.Count > 0)
            ? (AvailableCount / Required.Count)
            : 0;

        // The recipe’s overall limit is the smallest of these per-resource limits.
        MaxCraftable = FMath::Min(MaxCraftable, ThisItemAllows);
    }

    // If no requirements existed (MaxCraftable still Max int), clamp it down to 0.
    if (MaxCraftable == TNumericLimits<int32>::Max())
    {
        MaxCraftable = 0;
    }

    // Return the maximum number of complete recipe iterations possible.
    return MaxCraftable;
}

/**
 * Nomad Dev Team
 * Starts the crafting process:
 * - Sets count and recipe
 * - Stores instigator and target storage (for sending crafted items)
 * - Starts timer ticking CraftTick every 0.01 seconds
 */
void UACFCraftingComponent::StartCrafting(const FACFCraftingRecipe& Recipe, int32 Count, AACFCharacter* InstigatorCharacter, UACFStorageComponent* TargetStorage)
{
    if (Count <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Crafting] StartCrafting called with non-positive count: %d"), Count);
        return;
    }
    if (!InstigatorCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Crafting] StartCrafting called with null character"));
        return;
    }
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("[Crafting] StartCrafting without a valid World"));
        return;
    }
    
    RemainingCraftCount = Count;
    CurrentRecipe = Recipe;
    CurrentCraftProgress = 0.f;

    // Store instigator for later crafting calls
    CraftInstigator = InstigatorCharacter;

    // Store target storage (weak ptr to avoid invalid references)
    CurrentTargetStorage = TargetStorage;

    bIsCrating = true;  // <-- Set crafting active here

    // Begin timer calling CraftTick repeatedly
    GetWorld()->GetTimerManager().SetTimer(CraftTimerHandle, this, &UACFCraftingComponent::CraftTick, 0.01f, true);
}

/**
 * Nomad Dev Team
 * Cancels any ongoing crafting process immediately.
 * Stops the timer, resets all progress and counters, updates flags,
 * and notifies any listeners (e.g., UI) that crafting has been aborted.
 */
void UACFCraftingComponent::CancelCrafting()
{
    // 1. Stop the recurring timer that was driving CraftTick() calls.
    //    This prevents any further progress updates or item crafting from occurring.
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(CraftTimerHandle);
    }

    // 2. Reset the remaining craft count so no further items will be crafted.
    RemainingCraftCount = 0;

    // 3. Reset the current progress fraction to zero (0.0 = no progress).
    CurrentCraftProgress = 0.f;

    // 4. Flip the crafting state flag to false so IsCrafting() reports correctly.
    bIsCrating = false;

    // 5. Notify any bound UI or Blueprint widgets that progress has been reset.
    //    This ensures the progress bar visually returns to empty.
    OnCraftProgressUpdate.Broadcast(CurrentCraftProgress);

    // 6. (Optional) Notify listeners that crafting was explicitly canceled.
    //    You must define this delegate in the header:
    //      DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftCanceled);
    //      UPROPERTY(BlueprintAssignable) FOnCraftCanceled OnCraftCanceled;
    OnCraftCanceled.Broadcast();

    // 7. Log to the console for debugging purposes.
    UE_LOG(LogTemp, Log, TEXT("[UACFCraftingComponent] Crafting cancelled by user or system."));
}

/**
 * Nomad Dev Team
 * Tick function called on timer:
 * - Increments progress according to crafting time
 * - Broadcasts progress updates
 * - When progress reaches 1.0, completes one craft and resets progress
 * - Calls ItemsManager::CraftItem passing instigator and storage component
 */
void UACFCraftingComponent::CraftTick()
{
    // If nothing left to craft, stop timer and broadcast completion
    if (!bIsCrating || RemainingCraftCount <= 0)
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(CraftTimerHandle);
        }
        bIsCrating = false;
        OnCraftComplete.Broadcast();
        return;
    }

    // Advance progress
    if (CurrentRecipe.CraftingTime <= 0.f)
    {
        UE_LOG(LogTemp, Error, TEXT("[Crafting] Recipe has non-positive CraftingTime"));
        bIsCrating = false;
        return;
    }

    // Increase progress proportional to crafting speed
    CurrentCraftProgress += 0.01f / CurrentRecipe.CraftingTime;

    // Notify UI or listeners of current progress (0 to 1)
    OnCraftProgressUpdate.Broadcast(CurrentCraftProgress);

    // If craft finished
    if (CurrentCraftProgress >= 1.f)
    {
        bIsCrating = true;
        CurrentCraftProgress = 0.f;
        RemainingCraftCount--;

        // Get ItemsManager and call CraftItem with storage to store crafted item
        UACFItemsManagerComponent* ItemsManager = GetItemsManager();
        if (ItemsManager)
        {
            ItemsManager->CraftItem(CurrentRecipe, CraftInstigator.Get(), this, CurrentTargetStorage.Get());
        }

        // Reset progress broadcast after each item crafted
        OnCraftProgressUpdate.Broadcast(CurrentCraftProgress);
    }
}