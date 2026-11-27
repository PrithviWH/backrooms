// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "BackroomsPauseMenu.generated.h"

class UButton;

/**
 * Pause Menu Widget for Backrooms 5.8.
 * Liminal horror terminal aesthetic with Resume, Settings, and Quit functionality.
 */
UCLASS()
class BACKROOMS_API UBackroomsPauseMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UBackroomsPauseMenu(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	TObjectPtr<UButton> QuitButton;

	/** Widget class used to instantiate the Settings Menu */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> SettingsMenuClass;

	/** Current open settings menu instance */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> SettingsMenuInstance;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HandleResumeClicked();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HandleSettingsClicked();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HandleQuitClicked();

	/** Keyboard handling: Pressing Escape or Gamepad Start resumes gameplay */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FButtonStyle LiminalButtonStyle;
	FButtonStyle DangerButtonStyle;

	FButtonStyle CreateLiminalButtonStyle(bool bDanger = false) const;
	void PlayTactileClick();
};
