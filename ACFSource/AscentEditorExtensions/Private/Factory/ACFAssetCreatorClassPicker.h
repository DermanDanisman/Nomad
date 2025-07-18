﻿// Copyright (C) Developed by Pask & OlssonDev, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#pragma once
#include "ClassViewerFilter.h"

class UBlueprintFactory;

class SACFAssetCreatorClassPicker: public SCompoundWidget
{
	public:
	
	SLATE_BEGIN_ARGS(SACFAssetCreatorClassPicker){}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	/** Sets properties for the supplied BlueprintFactory */
	bool ConfigureProperties(TWeakObjectPtr<UBlueprintFactory> InBlueprintFactory);

	/** Creates the combo menu for the parent class */
	void MakeParentClassPicker();

	/** Handler for when a parent class is selected */
	void OnClassPicked(UClass* ChosenClass);

	/** Handler for when ok is clicked */
	FReply OkClicked();

	void CloseDialog(bool bWasPicked = false);

	/** Handler for when cancel is clicked */
	FReply CancelClicked();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:

	const FSlateBrush* GetBrush(FName PropertyName);
	const FMargin& GetMargin(FName PropertyName);
	float GetFloat(FName PropertyName);

	private:
	
	/** The factory for which we are setting up properties */
	TWeakObjectPtr<class UBlueprintFactory> BlueprintFactory;

	/** A pointer to the window that is asking the user to select a parent class */
	TWeakPtr<SWindow> PickerWindow;

	/** The container for the Parent Class picker */
	TSharedPtr<SVerticalBox> ParentClassContainer;

	/** The selected class */
	TWeakObjectPtr<UClass> ParentClass;

	/** True if Ok was clicked */
	bool bOkClicked;
	bool ClassPicked = false;
};

class AdventureClassPickerFilter : public IClassViewerFilter
{
public:
	/** All children of these classes will be included unless filtered out by another setting. */
	TSet<const UClass*> AllowedChildrenOfClasses;

	AdventureClassPickerFilter() {}

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override;
	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override;
};
