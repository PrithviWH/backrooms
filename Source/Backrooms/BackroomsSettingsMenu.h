// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "BackroomsSettingsMenu.generated.h"

class UButton;
class USlider;
class UTextBlock;
class UCheckBox;
class UWidgetSwitcher;
class UBackroomsSaveGame;
class UBackroomsPauseMenu;

/**
 * Settings Menu Widget for Backrooms 5.8.
 * Authentic liminal space horror terminal design featuring Audio, Graphics, and Controls configuration.
 */
UCLASS()
class BACKROOMS_API UBackroomsSettingsMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UBackroomsSettingsMenu(const FObjectInitializer& ObjectInitializer);

	// --- Tab Buttons ---
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Tabs")
	TObjectPtr<UButton> TabAudioButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Tabs")
	TObjectPtr<UButton> TabGraphicsButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Tabs")
	TObjectPtr<UButton> TabControlsButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Tabs")
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI")
	TObjectPtr<UButton> BackButton;

	// --- Audio Controls ---
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Audio")
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Audio")
	TObjectPtr<UTextBlock> MasterVolumeText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Audio")
	TObjectPtr<USlider> SFXVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Audio")
	TObjectPtr<UTextBlock> SFXVolumeText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Audio")
	TObjectPtr<USlider> AmbienceVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Audio")
	TObjectPtr<UTextBlock> AmbienceVolumeText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Audio")
	TObjectPtr<USlider> EntityVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Audio")
	TObjectPtr<UTextBlock> EntityVolumeText;

	// --- Graphics Controls ---
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Graphics")
	TObjectPtr<USlider> ResolutionScaleSlider;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Graphics")
	TObjectPtr<UTextBlock> ResolutionScaleText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Graphics")
	TObjectPtr<UTextBlock> FPSTextBlock;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Graphics")
	TObjectPtr<UButton> ApplyGraphicsButton;

	// --- Controls Options ---
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Controls")
	TObjectPtr<USlider> MouseSensitivitySlider;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Controls")
	TObjectPtr<UTextBlock> MouseSensitivityText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "UI|Controls")
	TObjectPtr<UCheckBox> InvertYCheckBox;

	/** Parent pause menu instance that spawned this settings menu */
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	TWeakObjectPtr<UBackroomsPauseMenu> ParentPauseMenu;

	// --- Logic & Callbacks ---
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SwitchToTab(int32 TabIndex);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HandleBackClicked();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnMasterVolumeChanged(float Value);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnSFXVolumeChanged(float Value);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnAmbienceVolumeChanged(float Value);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnEntityVolumeChanged(float Value);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnResolutionScaleChanged(float Value);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetQualityPreset(int32 QualityLevel);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ApplyGraphics();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnMouseSensitivityChanged(float Value);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnInvertYChanged(bool bIsChecked);

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	void LoadValuesFromSettings();
	void UpdateFPSCounter();

private:
	UPROPERTY()
	TObjectPtr<UBackroomsSaveGame> CurrentSettings;

	FTimerHandle FPSTimerHandle;

	int32 ActiveTabIndex;
	int32 SelectedQualityLevel;
	float PendingResolutionScale;
	float PendingMasterVolume;
	float PendingSFXVolume;
	float PendingAmbienceVolume;
	float PendingEntityVolume;
	float PendingSensitivity;
	bool bPendingInvertY;

	FButtonStyle CreateDarkButtonStyle(bool bSelected = false) const;
	TSharedRef<SWidget> BuildSlateAudioPanel();
	TSharedRef<SWidget> BuildSlateGraphicsPanel();
	TSharedRef<SWidget> BuildSlateControlsPanel();

	TSharedPtr<SWidget> SlateSwitcher;
	TSharedPtr<STextBlock> SlateFPSText;
	TSharedPtr<STextBlock> SlateResolutionText;
	TSharedPtr<STextBlock> SlateSensitivityText;
	TSharedPtr<STextBlock> SlateMasterText;
	TSharedPtr<STextBlock> SlateSFXText;
	TSharedPtr<STextBlock> SlateAmbienceText;
	TSharedPtr<STextBlock> SlateEntityText;

	// Dynamic tab header widgets
	TSharedPtr<SBorder> TabBorders[3];
	TSharedPtr<STextBlock> TabTexts[3];

	// Preset button widgets for segmented control
	TSharedPtr<SBorder> PresetBorders[4];
	TSharedPtr<STextBlock> PresetTexts[4];

	void UpdateTabVisuals();
	void UpdatePresetVisuals();
	void PlayTactileClick();
};
