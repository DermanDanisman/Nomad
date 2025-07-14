// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ALSLoadAndSaveSubsystem.h"
#include "ALSFunctionLibrary.h"
#include "ALSLoadAndSaveComponent.h"
#include "ALSSaveGameSettings.h"
#include "ALSSaveInfo.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include <GameFramework/Pawn.h>
#include <Serialization/MemoryReader.h>
#include <Serialization/MemoryWriter.h>

void UALSLoadAndSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UALSLoadAndSaveSubsystem::HandleLoadingFinished);
}

void UALSLoadAndSaveSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
}

void UALSLoadAndSaveSubsystem::SaveGameWorld(const FString& slotName, const FOnSaveFinished& saveCallback,
    const bool bSaveLocalPlayer /*= true*/, const bool bSaveScreenshot /*= true*/, const FString& slotDescription)
{
    if (systemState != ELoadingState::EIdle) {
        saveCallback.ExecuteIfBound(false);
        UE_LOG(LogTemp, Warning, TEXT("You are already loading or saving!"));
        return;
    }
    currentSaveSlot = slotName;
    onSaveFinishedInternal = saveCallback;
    currentSavegame = LoadOrCreateSaveGame(slotName);
    systemState = ELoadingState::ESaving;
    (new FAutoDeleteAsyncTask<FSaveWorldTask>(slotName, GetWorld(), bSaveLocalPlayer, bSaveScreenshot, slotDescription))->StartBackgroundTask();
}

void UALSLoadAndSaveSubsystem::SaveGameWorldInCurrentSlot(const FOnSaveFinished& saveCallback, const bool bSaveLocalPlayer /*= true*/,
    const bool bSaveScreenshot /*= true*/, const FString& slotDescription)
{
    SaveGameWorld(currentSaveSlot, saveCallback, bSaveLocalPlayer, bSaveScreenshot,slotDescription);
}

void UALSLoadAndSaveSubsystem::LoadGameWorld(const FString& slotName, const FOnLoadFinished& loadCallback)
{
    if (systemState != ELoadingState::EIdle) {
        loadCallback.ExecuteIfBound(false);
        UE_LOG(LogTemp, Warning, TEXT("You are already loading or saving!"));
        return;
    }

    FALSSaveMetadata saveToBeLoaded;
    if (TryGetSaveMetadata(slotName, saveToBeLoaded)) {
        currentSaveSlot = slotName;
        onLoadedCallbackInternal = loadCallback;
        SetLoadType(ELoadType::EFullReload);
        systemState = ELoadingState::ELoading;
       // CleanExtraActors();
        UGameplayStatics::OpenLevel(this, *saveToBeLoaded.MapToLoad);
    } else {
        loadCallback.ExecuteIfBound(false);
    }
}

bool UALSLoadAndSaveSubsystem::SaveLocalPlayer(const FString& slotName)
{
    APlayerController* playerCont = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    APawn* pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    UALSSaveInfo* saveInfo = LoadOrCreateSaveInfo();
    if (!saveInfo) {
        return false;
    }
    FALSSaveMetadata saveMetaData;
    saveMetaData.Data = FDateTime::Now();
    saveMetaData.SaveName = slotName;
    saveInfo->AddSlot(saveMetaData);
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    if (!saveSettings) {
        return false;
    }

    UALSSaveGame* saveGame = LoadOrCreateSaveGame(slotName);
    if (!saveGame) {
        return false;
    }

    const FALSActorData pcData = UALSFunctionLibrary::SerializeActor(playerCont);

    const FALSActorData pawnData = UALSFunctionLibrary::SerializeActor(pawn);

    FALSPlayerData playerData(pcData, pawnData);
    saveGame->StoreLocalPlayer(playerData);
    UGameplayStatics::SaveGameToSlot(saveGame, slotName, 0);
    UGameplayStatics::SaveGameToSlot(saveInfo, saveSettings->GetSaveMetadataName(), 0);
    currentSaveSlot = slotName;
    return true;
}

bool UALSLoadAndSaveSubsystem::SaveActor(AActor* actorToSave)
{
    if (actorToSave) {
        const FALSActorData pcData = UALSFunctionLibrary::SerializeActor(actorToSave);
        if (ExtraActors.Contains(pcData)) {
            ExtraActors.Remove(pcData);
        }
        ExtraActors.Add(pcData);
        return true;
    }
    return false;
}

bool UALSLoadAndSaveSubsystem::LoadActor(AActor* actorToLoad)
{
    if (!actorToLoad) {
        return false;
    }

    FALSActorData outActor;
    if (ExtraActors.Contains(actorToLoad->GetFName())) {
        outActor = *ExtraActors.FindByKey(actorToLoad->GetFName());
        UALSFunctionLibrary::DeserializeActor(actorToLoad, outActor);
        return true;
    } else if (currentSavegame) {
        if (currentSavegame->TryGetStoredWPActor(actorToLoad, outActor)) {
            UALSFunctionLibrary::DeserializeActor(actorToLoad, outActor);
            return true;
        }
    }
    return false;
}

bool UALSLoadAndSaveSubsystem::LoadLocalPlayer(const FString& slotName, bool bReloadTransform)
{
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();

    UALSSaveGame* saveGame = LoadOrCreateSaveGame(slotName);
    if (!saveGame) {
        return false;
    }

    if (!saveSettings) {
        return false;
    }
    FALSPlayerData outData;
    saveGame->GetLocalPlayer(outData);
    APlayerController* playerCont = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    APawn* pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    FALSActorData playerData;

    UALSFunctionLibrary::FullDeserializeActor(pawn,
        outData.Pawn, bReloadTransform);
    UALSFunctionLibrary::FullDeserializeActor(playerCont,
        outData.PlayerController, bReloadTransform);
    currentSaveSlot = slotName;
    return true;
}

bool UALSLoadAndSaveSubsystem::SavePlayer(const FString& slotName, const FString& playerID, APlayerController* playerToSave)
{
    UALSSaveGame* saveGame = LoadOrCreateSaveGame(slotName);
    if (!saveGame) {
        return false;
    }
    const FALSActorData pcData = UALSFunctionLibrary::SerializeActor(playerToSave);
    APawn* pawn = playerToSave->GetPawn();
    const FALSActorData pawnData = UALSFunctionLibrary::SerializeActor(pawn);

    FALSPlayerData playerData(pcData, pawnData);
    saveGame->StorePlayer(slotName, playerData);
    CreateOrUpdateSlotInfo(slotName);
    currentSaveSlot = slotName;

    return UGameplayStatics::SaveGameToSlot(saveGame, slotName, 0);
}

bool UALSLoadAndSaveSubsystem::CreateOrUpdateSlotInfo(const FString& slotName)
{
    UALSSaveInfo* saveInfo = LoadOrCreateSaveInfo();
    if (!saveInfo) {
        return false;
    }
    FALSSaveMetadata saveMetaData;
    saveMetaData.Data = FDateTime::Now();
    saveMetaData.SaveName = slotName;

    saveInfo->AddSlot(saveMetaData);
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    if (!saveSettings) {
        return false;
    }
    return UGameplayStatics::SaveGameToSlot(saveInfo, saveSettings->GetSaveMetadataName(), 0);
}

bool UALSLoadAndSaveSubsystem::RemoveSlotInfo(const FString& slotName)
{
    UALSSaveInfo* saveInfo = LoadOrCreateSaveInfo();
    if (!saveInfo) {
        return false;
    }

    saveInfo->DeleteSlot(slotName);
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    if (!saveSettings) {
        return false;
    }
    return UGameplayStatics::SaveGameToSlot(saveInfo, saveSettings->GetSaveMetadataName(), 0);
}

bool UALSLoadAndSaveSubsystem::LoadPlayer(const FString& slotName, const FString& playerID, APlayerController* playerToLoad, bool bReloadTransform)
{
    UALSSaveGame* saveGame = LoadOrCreateSaveGame(slotName);
    if (!saveGame) {
        return false;
    }
    if (!playerToLoad) {
        return false;
    }

    FALSPlayerData outData;
    if (saveGame->TryGetPlayer(playerID, outData)) {
        UALSFunctionLibrary::FullDeserializeActor(playerToLoad, outData.PlayerController, bReloadTransform);
        UALSFunctionLibrary::FullDeserializeActor(playerToLoad->GetPawn(), outData.Pawn, bReloadTransform);
        currentSaveSlot = slotName;
        return true;
    } else {
        UE_LOG(LogTemp, Warning, TEXT("There are no Players with the provided ID in SaveSlot !"));
        return false;
    }
}

void UALSLoadAndSaveSubsystem::LoadCurrentLevel(const FString& slotName, const FOnLoadFinished& loadCallback, bool bReloadCrossLevelActors /*= false*/)
{
    const FString levelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
    LoadLevelFromSaveGame(slotName, levelName, loadCallback, bReloadCrossLevelActors);
}

void UALSLoadAndSaveSubsystem::LoadLevelFromSaveGame(const FString& slotName, const FString& levelName, const FOnLoadFinished& loadCallback, bool bReloadCrossLevelActors /*= false*/)
{
    if (systemState != ELoadingState::EIdle) {
        loadCallback.ExecuteIfBound(false);
        UE_LOG(LogTemp, Warning, TEXT("You are already loading or saving!"));
        return;
    }

    FALSSaveMetadata saveToBeLoaded;
    if (TryGetSaveMetadata(slotName, saveToBeLoaded)) {
        onLoadedCallbackInternal = loadCallback;
        SetLoadType(ELoadType::EFullReload);
        systemState = ELoadingState::ELoading;
        currentSaveSlot = slotName;
        AsyncLoadSaveGame(currentSaveSlot);
    } else {
        loadCallback.ExecuteIfBound(false);
    }
}

bool UALSLoadAndSaveSubsystem::TravelLocalPlayer()
{
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    SetLoadType(ELoadType::EPlayerOnly);
    return SaveLocalPlayer(saveSettings->GetTravelSaveName());
}

bool UALSLoadAndSaveSubsystem::LoadTraveledPlayer()
{
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    const FString tempSlot = saveSettings->GetTravelSaveName();
    if (LoadLocalPlayer(tempSlot, false)) {
        UGameplayStatics::DeleteGameInSlot(tempSlot, 0);
        RemoveSlotInfo(tempSlot);
        SetLoadType(ELoadType::EDontReload);
        return true;
    }
    return false;
}

bool UALSLoadAndSaveSubsystem::TryGetSaveMetadata(const FString& slotName, FALSSaveMetadata& outSaveMetadata) const
{
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    const UALSSaveInfo* saveMetadata = Cast<UALSSaveInfo>(UGameplayStatics::LoadGameFromSlot(saveSettings->GetSaveMetadataName(), 0));

    if (!saveMetadata) {
        return false;
    }

    return saveMetadata->TryGetSaveSlotData(slotName, outSaveMetadata); 
}

TArray<FALSSaveMetadata> UALSLoadAndSaveSubsystem::GetAllSaveGames() const
{
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    const UALSSaveInfo* saveMetadata = Cast<UALSSaveInfo>(UGameplayStatics::LoadGameFromSlot(saveSettings->GetSaveMetadataName(), 0));

    if (saveMetadata) {
        return saveMetadata->GetSaveSlots();
    }

    return TArray<FALSSaveMetadata>();
}

bool UALSLoadAndSaveSubsystem::HasAnySaveGame() const
{
    return GetAllSaveGames().Num() > 0;
}

int32 UALSLoadAndSaveSubsystem::GetCurrentSlotNum() const
{
    return GetAllSaveGames().Num();
}

int32 UALSLoadAndSaveSubsystem::GetMaxSlotsNum() const
{
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();

    if (saveSettings) {
        return saveSettings->GetMaxSlotsNum();
    }

    return -1;
}


bool UALSLoadAndSaveSubsystem::CanAddNewSlot() const
{
    return GetAllSaveGames().Num() < GetMaxSlotsNum();
}

bool UALSLoadAndSaveSubsystem::IsSlotNameUnique(const FString& slotName) const
{
    return !GetAllSaveGames().Contains(slotName);
}

UTexture2D* UALSLoadAndSaveSubsystem::GetScreenshotForSave(const FString& saveName) const
{
    return UALSFunctionLibrary::GetScreenshotByName(saveName);
}

FString UALSLoadAndSaveSubsystem::GetDefaultSaveName() const
{
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    return saveSettings->GetDefaultSaveName();
}

void UALSLoadAndSaveSubsystem::FinishSaveWork(const bool bSuccess)
{
    onSaveFinishedInternal.ExecuteIfBound(bSuccess);
    systemState = ELoadingState::EIdle;
}

void UALSLoadAndSaveSubsystem::FinishLoadWork(const bool bSuccess)
{

    onLoadedCallbackInternal.ExecuteIfBound(bSuccess);

    systemState = ELoadingState::EIdle;
}

void UALSLoadAndSaveSubsystem::HandleLoadingFinished(UWorld* world)
{
    if (systemState == ELoadingState::ELoading) {
        AsyncLoadSaveGame(currentSaveSlot);
    }
}

void UALSLoadAndSaveSubsystem::HandleLoadCompleted(const FString& SaveSlot, const int32 UserIndex, USaveGame* LoadedSaveData)
{
    if (!LoadedSaveData) {
        onLoadedCallbackInternal.ExecuteIfBound(false);
        systemState = ELoadingState::EIdle;
        return;
    }
    currentSaveSlot = SaveSlot;
    currentSavegame = Cast<UALSSaveGame>(LoadedSaveData);
    if (!currentSavegame) {
        onLoadedCallbackInternal.ExecuteIfBound(false);
        systemState = ELoadingState::EIdle;
        SetLoadType(ELoadType::EDontReload);
        return;
    }
    (new FAsyncTask<FLoadWorldTask>(currentSaveSlot, GetWorld(), UGameplayStatics::GetCurrentLevelName(GetWorld()), true))->StartBackgroundTask();
}

void UALSLoadAndSaveSubsystem::AsyncLoadSaveGame(const FString& savegameName)
{
    FAsyncLoadGameFromSlotDelegate LoadedDelegate;
    LoadedDelegate.BindUObject(this, &UALSLoadAndSaveSubsystem::HandleLoadCompleted);

    UGameplayStatics::AsyncLoadGameFromSlot(savegameName, 0, LoadedDelegate);
}

void UALSLoadAndSaveSubsystem::SerializeObject(UObject* objectToSerialize, FALSObjectData& outData)
{
    FALSObjectData Record = { objectToSerialize };

    FMemoryWriter MemoryWriter(Record.Data, true);
    FALSSaveGameArchive Archive(MemoryWriter, false);
    objectToSerialize->Serialize(Archive);
    outData = Record;
}

void UALSLoadAndSaveSubsystem::DeserializeObject(UObject* settingsObject, const FALSObjectData& objectData)
{
    FMemoryReader MemoryReader(objectData.Data, true);
    FALSSaveGameArchive Archive(MemoryReader, false);
    settingsObject->Serialize(Archive);
}

UALSSaveGame* UALSLoadAndSaveSubsystem::LoadOrCreateSaveGame(const FString& slotName)
{
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    UALSSaveGame* saveGame = Cast<UALSSaveGame>(UGameplayStatics::LoadGameFromSlot(slotName, 0));
    if (saveGame) {
        return saveGame;
    }

    return Cast<UALSSaveGame>(UGameplayStatics::CreateSaveGameObject(saveSettings->GetSaveGameClass()));
}

UALSSaveInfo* UALSLoadAndSaveSubsystem::LoadOrCreateSaveInfo()
{
    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    UALSSaveInfo* saveMetadata = Cast<UALSSaveInfo>(UGameplayStatics::LoadGameFromSlot(saveSettings->GetSaveMetadataName(), 0));
    if (saveMetadata) {
        return saveMetadata;
    }

    return Cast<UALSSaveInfo>(UGameplayStatics::CreateSaveGameObject(UALSSaveInfo::StaticClass()));
}
