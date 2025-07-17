// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#include "AscentEditorExtensions.h"
#include "Logging.h"

#include "Modules/ModuleManager.h"
#include "ACFEditorStyle.h"




#define LOCTEXT_NAMESPACE "FACFEditorExtensions"




void FAscentEditorExtensions::StartupModule()
{
	FACFEditorStyle::Initialize();
	FACFEditorStyle::ReloadTextures();
}

void FAscentEditorExtensions::ShutdownModule()
{
	FACFEditorStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAscentEditorExtensions, AscentEditorExtensions);