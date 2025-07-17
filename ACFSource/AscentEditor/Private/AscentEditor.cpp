// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AscentEditor.h"
#include "AscentEditorStyle.h"
#include "AscentEditorCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "ACFActionsBlueprint.h"
#include "ACFActionsSetBlueprint.h"
#include "Actions/ACFActionsSet.h"
#include "Actions/ACFBaseAction.h"
#include "AssetTypeActions/AssetTypeActions_Blueprint.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_Base.h"
#include "AssetTypeCategories.h"

static const FName AscentEditorTabName("AscentEditor");

#define LOCTEXT_NAMESPACE "FAscentEditorModule"


class FAssetTypeActions_ACFAction : public FAssetTypeActions_Blueprint
{
public:

	FAssetTypeActions_ACFAction(EAssetTypeCategories::Type InAssetCategory) {
		MyAssetCategory = InAssetCategory;
	 };

	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_ACFGameObject", "ACF"); }
	virtual UClass* GetSupportedClass() const override { return UACFActionsBlueprint::StaticClass(); }
	virtual uint32 GetCategories() override { return MyAssetCategory; }
	virtual FColor GetTypeColor() const override {
		return FColor(80, 80, 220, 255);
	};


private:
	EAssetTypeCategories::Type MyAssetCategory;
};

class FAssetTypeActions_ACFActionSet : public FAssetTypeActions_Blueprint
{
public:

	FAssetTypeActions_ACFActionSet(EAssetTypeCategories::Type InAssetCategory) {
		MyAssetCategory = InAssetCategory;
	};

	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActionSet", "AssetTypeActions_ACFActionsSet", "ACF"); }
	virtual UClass* GetSupportedClass() const override { return UACFActionsSetBlueprint::StaticClass(); }
	virtual uint32 GetCategories() override { return MyAssetCategory; }
	//virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual FColor GetTypeColor() const override {
		return FColor(80, 80, 220, 255);
	};


private:
	EAssetTypeCategories::Type MyAssetCategory;
};



void FAscentEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FAscentEditorStyle::Initialize();
	FAscentEditorStyle::ReloadTextures();

	FAscentEditorCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FAscentEditorCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FAscentEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAscentEditorModule::RegisterMenus));
	// Register asset types
// 	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
// 	{
// 		ACFGraphAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("ACF")), LOCTEXT("GraphAssetCategory", "ACF"));
// 		TSharedRef<IAssetTypeActions> Action = MakeShareable(new FAssetTypeActions_ACFAction(ACFGraphAssetCategory));
// 		TSharedRef<IAssetTypeActions> ActionSet = MakeShareable(new FAssetTypeActions_ACFActionSet(ACFGraphAssetCategory));
// 
// 		AssetTools.RegisterAssetTypeActions(Action);
// 		AssetTools.RegisterAssetTypeActions(ActionSet);
// 	}

}

void FAscentEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FAscentEditorStyle::Shutdown();

	FAscentEditorCommands::Unregister();
}

void FAscentEditorModule::PluginButtonClicked()
{
	   // Put your "OnButtonClicked" stuff here
    UEditorUtilityWidgetBlueprint* editorWidget = LoadObject<UEditorUtilityWidgetBlueprint>(nullptr, TEXT("/AscentCombatFramework/Editor/ACFAssistant_EUW")); 

    if (editorWidget) {
        UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
		EditorUtilitySubsystem->SpawnAndRegisterTab(editorWidget);
    } else {
          	FText DialogText = FText::Format(
							LOCTEXT("PluginButtonDialogText", "Missing ACF Editor Widget"),
							FText::FromString(TEXT("FAscentEditorModule::PluginButtonClicked()")),
							FText::FromString(TEXT("AscentEditor.cpp"))
					   );
	FMessageDialog::Open(EAppMsgType::Ok, DialogText);
    }

}

void FAscentEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FAscentEditorCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("ACFTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FAscentEditorCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAscentEditorModule, AscentEditor)