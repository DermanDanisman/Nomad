// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ALSFunctionLibrary.h"
#include "ALSLoadTask.h"
#include "ALSSaveGameSettings.h"
#include "ALSSaveInfo.h"
#include "ALSSaveTask.h"
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include <Async/AsyncWork.h>
#include <Engine/World.h>
#include "ALSLoadAndSaveComponent.h"

#include "ALSLoadAndSaveSubsystem.generated.h"

/**
 *
 */
UENUM(BlueprintType)
enum class ELoadingState : uint8 {
    EIdle,
    ESaving,
    ELoading
};

UCLASS()
class ASCENTSAVESYSTEM_API UALSLoadAndSaveSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    /** Implement this for initialization of instances of the system */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Implement this for deinitialization of instances of the system */
    virtual void Deinitialize() override;

    /*Saves all the game world ina save game*/
    UFUNCTION(BlueprintCallable, Category = ALS)
    void SaveGameWorld(const FString& slotName, const FOnSaveFinished& saveCallback, const bool bSaveLocalPlayer = true, 
        const bool bSaveScreenshot = true, const FString& slotDescription = "");

    /*Saves all the game world ina save game*/
    UFUNCTION(BlueprintCallable, Category = ALS)
    void SaveGameWorldInCurrentSlot(const FOnSaveFinished& saveCallback, const bool bSaveLocalPlayer = true, 
        const bool bSaveScreenshot = true, const FString& slotDescription = "");

    /*Load the provided slot and open the saved map*/
    UFUNCTION(BlueprintCallable, Category = ALS)
    void LoadGameWorld(const FString& slotName, const FOnLoadFinished& loadCallback);

    /*Reloads current level from the provided slot*/
    UFUNCTION(BlueprintCallable, Category = ALS)
    void LoadCurrentLevel(const FString& slotName, const FOnLoadFinished& loadCallback, bool bReloadLocalPlayer = true);

    /*Loads the provided level from the provided slot*/
    UFUNCTION(BlueprintCallable, Category = ALS)
    void LoadLevelFromSaveGame(const FString& slotName, const FString& levelName, const FOnLoadFinished& loadCallback, bool bReloadLocalPlayer);

    /*Save Current Player Controller & Pawn in a temporary slot, useful for map travel*/
    UFUNCTION(BlueprintCallable, Category = ALS)
    bool TravelLocalPlayer();

    /*Reloads the player from the Travel slot. Usable only after TravelLocalPlayer*/
    UFUNCTION(BlueprintCallable, Category = ALS)
    bool LoadTraveledPlayer();

    /*Save Current Player Controller & Pawn in provided slot*/
    UFUNCTION(BlueprintCallable, Category = ALS)
    bool SaveLocalPlayer(const FString& slotName);

    UFUNCTION(BlueprintCallable, Category = ALS)
    bool SaveActor(AActor* actorToSave);

   UFUNCTION(BlueprintCallable, Category = ALS)
    bool LoadActor( AActor* actorToLoad);

    /*Reloads the player from the provided slot*/
    UFUNCTION(BlueprintCallable, Category = ALS)
    bool LoadLocalPlayer(const FString& slotName, bool bReloadTransform);

    /*Save Player Controller & Pawn in provided slot. Usefull for multiplayer Games*/
    UFUNCTION(BlueprintCallable, Category = ALS)
    bool SavePlayer(const FString& slotName, const FString& playerID, APlayerController* playerToSave);

    bool CreateOrUpdateSlotInfo(const FString& slotName);
    bool RemoveSlotInfo(const FString& slotName);


    /*Reloads the player from the provided slot Usefull for multiplayer Games */
    UFUNCTION(BlueprintCallable, Category = ALS)
    bool LoadPlayer(const FString& slotName, const FString& playerID, APlayerController* playerToLoad, bool bReloadTransform);

    UFUNCTION(BlueprintCallable, Category = ALS)
    void SetLoadType(ELoadType inLoadType)
    {
        loadType = inLoadType;
    }

    UFUNCTION(BlueprintPure, Category = ALS)
    bool IsNewGame() const
    {
        return loadType  == ELoadType::EDontReload;
    }

    UFUNCTION(BlueprintPure, Category = ALS)
    ELoadType GetLoadType() const {
        return loadType;
    }

    UFUNCTION(BlueprintCallable, Category = ALS)
    bool TryGetSaveMetadata(const FString& slotName, FALSSaveMetadata& outSaveMetadata) const;

    UFUNCTION(BlueprintCallable, Category = ALS)
    class UALSSaveGame* LoadOrCreateSaveGame(const FString& slotName);

    UFUNCTION(BlueprintCallable, Category = ALS)
    class UALSSaveInfo* LoadOrCreateSaveInfo();

    UFUNCTION(BlueprintPure, Category = ALS)
    TArray<FALSSaveMetadata> GetAllSaveGames() const;

    UFUNCTION(BlueprintPure, Category = ALS)
    bool HasAnySaveGame() const;

    UFUNCTION(BlueprintPure, Category = ALS)
    int32 GetCurrentSlotNum() const;

    UFUNCTION(BlueprintPure, Category = ALS)
	int32 GetMaxSlotsNum() const;

    UFUNCTION(BlueprintPure, Category = ALS)
    bool CanAddNewSlot() const;

    UFUNCTION(BlueprintCallable, Category = ALS)
    bool IsSlotNameUnique(const FString& slotName) const;

    UFUNCTION(BlueprintCallable, Category = ALS)
    UTexture2D* GetScreenshotForSave(const FString& saveName) const;

    UFUNCTION(BlueprintPure, Category = ALS)
    ELoadingState GetSystemState() const
    {
        return systemState;
    }

    UFUNCTION(BlueprintPure, Category = ALS)
    FString GetCurrentSaveName() const
    {
        return currentSaveSlot;
    }

     UFUNCTION(BlueprintPure, Category = ALS)
    FString GetDefaultSaveName() const;

    UFUNCTION(BlueprintPure, Category = ALS)
    class UALSSaveGame* GetCurrentSaveGame() const
    {
        return currentSavegame;
    }

    void GetExtraActors(TArray<FALSActorData>& outActors) const
    {
        outActors = ExtraActors;
    }

    UFUNCTION(BlueprintCallable, Category = ALS)
    void CleanExtraActors() {
        ExtraActors.Empty();
    }

    void FinishSaveWork(const bool bSuccess);

    void FinishLoadWork(const bool bSuccess);

private:
    ELoadType loadType = ELoadType::EDontReload;
    bool bIsLoading;

    UFUNCTION()
    void HandleLoadingFinished(UWorld* world);

    UFUNCTION()
    void HandleLoadCompleted(const FString& SaveSlot, const int32 UserIndex, USaveGame* LoadedSaveData);

    void AsyncLoadSaveGame(const FString& savegameName);

    void SerializeObject(UObject* objectToSerialize, FALSObjectData& outData);
    void DeserializeObject(UObject* settingsObject, const FALSObjectData& objectData);

    FOnLoadFinished onLoadedCallbackInternal;
    FOnSaveFinished onSaveFinishedInternal;

    UPROPERTY()
    ELoadingState systemState;

    FString currentSaveSlot;
    UPROPERTY()
    class UALSSaveGame* currentSavegame;

    
    UPROPERTY()
    TArray<FALSActorData> ExtraActors;
};

static void GFinishSave(UWorld* WorldContextObject, bool bSuccess)
{
    UGameplayStatics::GetGameInstance(WorldContextObject)->GetSubsystem<UALSLoadAndSaveSubsystem>()->FinishSaveWork(bSuccess);
}

static void GFinishLoad(UWorld* WorldContextObject,  TArray<FALSActorData> ToBeSpawned,
    TMap<AActor*, FALSActorLoaded> SavedActors, TArray<AActor*> ToBeDestroyed, TArray<UALSLoadAndSaveComponent*> wpActors, bool bSuccess)
{
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    if (bSuccess && saveSettings) {
        for (const auto& actor : ToBeDestroyed) {
            actor->Destroy();
        }
        FActorSpawnParameters SpawnInfo {};
        for (const auto& Record : ToBeSpawned) {
            // SpawnInfo.Name = Record.alsName;
            AActor* spawnedActor = WorldContextObject->SpawnActor(Record.Class, &Record.Transform, SpawnInfo);
            if (spawnedActor) {
                UALSFunctionLibrary::DeserializeActor(spawnedActor, Record);

                UALSFunctionLibrary::ExecuteFunctionsOnSavableActor(spawnedActor,
                    saveSettings->GetOnComponentLoadedFunctionName());
            }
        }

        for (const auto& actorRec : SavedActors) {
            if (!actorRec.Key) {
                continue;
            }
            actorRec.Key->SetActorTransform(actorRec.Value.transform);

            UALSFunctionLibrary::ExecuteFunctionsOnSavableActor(actorRec.Key,
                saveSettings->GetOnComponentLoadedFunctionName());
        }

        for (const auto& comp : wpActors) {
            comp->DispatchLoaded();
        }
    }
    UGameplayStatics::GetGameInstance(WorldContextObject)->GetSubsystem<UALSLoadAndSaveSubsystem>()->FinishLoadWork(bSuccess);
}
