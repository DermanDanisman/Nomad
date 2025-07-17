// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.


#include "ANSDeveloperSettings.h"

UDataTable* UANSDeveloperSettings::GetIconsByTagDT() const
{
    return Cast<UDataTable>(IconsByTag.TryLoad());
}

UDataTable* UANSDeveloperSettings::GetKeysConfigByPlatformDT(const FString& platformName) const
{
    const FSoftObjectPath* dtref = KeysConfigByPlatform.Find(platformName);
    if (dtref) {
        return Cast<UDataTable>(dtref->TryLoad());   
    }
    return nullptr;
}
