#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "WebPTestWidget.generated.h" // Make sure this matches your filename

// Forward declare UImage and UButton if you want to bind them from C++
// (Alternatively, you can handle button clicks and image updates purely in Blueprint
// by calling a UFUNCTION from Blueprint, which is often simpler for EUWs)
class UImage;
class UButton;

UCLASS()
class VNM_API UWebPTestWidget : public UEditorUtilityWidget // Replace YOURPROJECT_API with your project's API macro
{
	GENERATED_BODY()

public:
	// This function will be called from the Blueprint when the button is clicked.
	UFUNCTION(BlueprintCallable, Category = "WebP Test")
	void PerformWebPTest();

	// This UImage will be bound to the Image widget in the Blueprint so C++ can update it.
	// Ensure its name here matches the "Variable Name" you give it in the Blueprint Designer (IsVariable=true)
	UPROPERTY(meta = (BindWidgetOptional)) // Use BindWidgetOptional if you might not always have it or want to check
	UImage* DisplayedImage;

	// You can also bind the button if you want to handle its OnClicked from C++
	// UPROPERTY(meta = (BindWidget))
	// UButton* TestButton;

protected:
	// virtual void NativeConstruct() override; // If you need to bind button OnClicked in C++
};