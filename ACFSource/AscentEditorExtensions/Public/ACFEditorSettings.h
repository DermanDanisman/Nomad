// Copyright (C) Developed by Pask & OlssonDev, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ACFEditorTypes.h"

#include "ACFEditorSettings.generated.h"

UCLASS(config = Plugins, Defaultconfig, meta = (DisplayName = "Ascent Editor Settings"))
class ASCENTEDITOREXTENSIONS_API UACFEditorSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
    FName GetStyleName() const { return StyleName; }
    TArray<FPlaceCategoryConfig> GetQuickPlacementActor() const { return QuickPlacementActor; }
    bool GetScanOnlyAssignedFolders() const { return ScanOnlyAssignedFolders; }
    TArray<FString> GetAssetPaths() const { return AssetPaths; }
  //  TArray<FMainCategoryConfig> GetContentBrowserConfig() const { return ContentBrowserConfig; }

protected:
    UPROPERTY(EditAnywhere, config, Category = "Placement Mode")
    FName StyleName = "AscentEditorStyle";

    UPROPERTY(EditAnywhere, config, Category = "Placement Mode")
    TArray<FPlaceCategoryConfig> QuickPlacementActor;

//     UPROPERTY(EditAnywhere, config, Category = "Content Browser")
//     TArray<FMainCategoryConfig> ContentBrowserConfig;

    // If the plugin should only scan certain folders for Assets/AssetActions/AssetFactory
    UPROPERTY(EditDefaultsOnly, Category = "Asset Scanning", Config)
    bool ScanOnlyAssignedFolders = false;

    // The paths to scan
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = ScanOnlyAssignedFolders), Category = "Asset Scanning", Config)
    TArray<FString> AssetPaths;
};
