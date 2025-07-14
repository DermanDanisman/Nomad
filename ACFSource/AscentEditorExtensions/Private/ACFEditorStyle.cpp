// Copyright Epic Games, Inc. All Rights Reserved.

#include "ACFEditorStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Styling/SlateStyleMacros.h"
#include <Interfaces/IPluginManager.h>

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FACFEditorStyle::StyleInstance = nullptr;


void FACFEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FACFEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FACFEditorStyle::GetStyleSetName()
{
	return StyleSetName;
}

const FVector2D Icon32x32(32.0f, 32.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FACFEditorStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet(StyleSetName));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin(PluginName)->GetBaseDir() / TEXT("Resources"));

	Style->Set("ACFEditor.SmallIcon", new IMAGE_BRUSH(TEXT("ACF32"), Icon20x20));

	return Style;
}

void FACFEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FACFEditorStyle::Get()
{
	return *StyleInstance;
}
