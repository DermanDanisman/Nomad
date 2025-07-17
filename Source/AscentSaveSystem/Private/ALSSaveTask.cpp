// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ALSSaveTask.h"
#include "ALSFunctionLibrary.h"
#include "ALSLoadAndSaveSubsystem.h"
#include "ALSSavableInterface.h"
#include "ALSSaveGame.h"
#include "ALSSaveGameSettings.h"
#include "ALSSaveInfo.h"
#include "ALSSaveTypes.h"
#include "Engine/LevelStreaming.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include <Async/TaskGraphInterfaces.h>

void FSaveWorldTask::DoWork()
{
    if (!world) {
        FinishSave(false);
        return;
    }

    const UALSSaveGameSettings* saveSettings = GetMutableDefault<UALSSaveGameSettings>();
    if (!saveSettings) {
        FinishSave(false);
        return;
    }
    UALSLoadAndSaveSubsystem* saveSusbsystem = UGameplayStatics::GetGameInstance(world)->GetSubsystem<UALSLoadAndSaveSubsystem>();

    const TSubclassOf<UALSSaveGame> saveClass = saveSettings->GetSaveGameClass();
    if (!saveClass) {
        FinishSave(false);
        return;
    }
    newSave = saveSusbsystem->GetCurrentSaveGame(); // Cast<UALSSaveGame>(UGameplayStatics::CreateSaveGameObject(saveClass));

    FALSLevelData currentLevel;

    for (const auto& actor : SavableActors) {
        if (!actor) {
            continue;
        }
        if (!UALSFunctionLibrary::ShouldSaveActor(actor)) {
            continue;
        }
        if (UALSFunctionLibrary::IsSpecialActor(world, actor)) {
            continue;
        }
        FALSActorData actorData = SerializeActor(actor);
        currentLevel.AddActorRecord(actorData);
    }
    UALSSaveInfo* saveInfo = saveSusbsystem->LoadOrCreateSaveInfo();
    if (!saveInfo) {
        FinishSave(false);
    }

    FALSSaveMetadata saveMetaData;
    saveMetaData.MapToLoad = UGameplayStatics::GetCurrentLevelName(world, true);
    saveMetaData.Data = FDateTime::Now();
    saveMetaData.SaveName = saveName;
    saveMetaData.SaveDescription = slotDesc;
    saveInfo->AddSlot(saveMetaData);

    newSave->AddLevel(saveMetaData.MapToLoad, currentLevel);

    TArray<FALSActorData> outActors;
    saveSusbsystem->GetExtraActors(outActors);

    for (const auto& extActor : outActors) {
        newSave->StoreWPActors(extActor);
    }
  //  saveSusbsystem->CleanExtraActors();
    StoreLocalPlayer();

    newSave->OnSaved();
    UGameplayStatics::SaveGameToSlot(newSave, saveName, 0);

    if (bSaveScreenshot) {
        UALSFunctionLibrary::TrySaveSceenshot(saveName, saveSettings->GetDefaultScreenshotWidth(), saveSettings->GetDefaultScreenshotHeight());
    }
    UGameplayStatics::SaveGameToSlot(saveInfo, saveSettings->GetSaveMetadataName(), 0);

    FinishSave(true);
}

FALSActorData FSaveWorldTask::SerializeActor(AActor* actor)
{
    FALSActorData outData = UALSFunctionLibrary::SerializeActor(actor);
    SuccessfullySavedActors.Add(actor);
    return outData;
}

void FSaveWorldTask::FinishSave(const bool bSuccess)
{
    if (IsInGameThread()) {
        UGameplayStatics::GetGameInstance(this->world)->GetSubsystem<UALSLoadAndSaveSubsystem>()->FinishSaveWork(bSuccess);
    } else {
        FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
            FSimpleDelegateGraphTask::FDelegate::CreateStatic(
                &GFinishSave, world,
                bSuccess),
            GetStatId(),
            nullptr, ENamedThreads::GameThread);
    }
}

void FSaveWorldTask::StoreLocalPlayer()
{
    APlayerController* playerCont = UGameplayStatics::GetPlayerController(world, 0);
    APawn* pawn = UGameplayStatics::GetPlayerPawn(world, 0);
    if (UALSFunctionLibrary::ShouldSaveActor(playerCont) && UALSFunctionLibrary::ShouldSaveActor(pawn)) {
        const FALSActorData pcData = SerializeActor(playerCont);
        const FALSActorData pawnData = SerializeActor(pawn);
        FALSPlayerData playerData(pcData, pawnData);
        newSave->StoreLocalPlayer(playerData);
    } else {
        UE_LOG(LogTemp, Error,
            TEXT("Player Controller or Pawn does not implement savable interface! - FSaveWorldTask::StoreLocalPlayer"));
    }
}
