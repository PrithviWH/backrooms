// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsSettingsMenu.h"
#include "BackroomsPauseMenu.h"
#include "BackroomsSaveGame.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SCheckBox.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Sound/SoundBase.h"

UBackroomsSettingsMenu::UBackroomsSettingsMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ActiveTabIndex(0)
	, SelectedQualityLevel(2)
	, PendingResolutionScale(100.0f)
	, PendingMasterVolume(0.70f)
	, PendingSFXVolume(0.80f)
	, PendingAmbienceVolume(0.60f)
	, PendingEntityVolume(0.80f)
	, PendingSensitivity(1.0f)
	, bPendingInvertY(false)
{
	SetIsFocusable(true);
}

FButtonStyle UBackroomsSettingsMenu::CreateDarkButtonStyle(bool bSelected) const
{
	FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");

	FLinearColor NormalCol = bSelected ? FLinearColor(42.0f / 255.0f, 42.0f / 255.0f, 36.0f / 255.0f, 1.0f)
	                                   : FLinearColor(24.0f / 255.0f, 24.0f / 255.0f, 22.0f / 255.0f, 0.95f);
	FLinearColor HoverCol  = FLinearColor(60.0f / 255.0f, 60.0f / 255.0f, 50.0f / 255.0f, 1.0f);
	FLinearColor PressedCol = FLinearColor(18.0f / 255.0f, 18.0f / 255.0f, 16.0f / 255.0f, 1.0f);

	Style.Normal.TintColor = FSlateColor(NormalCol);
	Style.Hovered.TintColor = FSlateColor(HoverCol);
	Style.Pressed.TintColor = FSlateColor(PressedCol);

	return Style;
}

void UBackroomsSettingsMenu::PlayTactileClick()
{
	static USoundBase* CachedClickSound = nullptr;
	if (!CachedClickSound)
	{
		CachedClickSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Backrooms/Audio/freesound_community-quiet-thud-hits-100179.freesound_community-quiet-thud-hits-100179"));
	}
	if (CachedClickSound)
	{
		UGameplayStatics::PlaySound2D(this, CachedClickSound, 0.45f, FMath::FRandRange(0.96f, 1.04f));
	}
}

void UBackroomsSettingsMenu::NativeConstruct()
{
	Super::NativeConstruct();

	LoadValuesFromSettings();

	if (TabAudioButton) { TabAudioButton->OnClicked.AddDynamic(this, &UBackroomsSettingsMenu::HandleBackClicked); }
	if (BackButton) { BackButton->OnClicked.AddDynamic(this, &UBackroomsSettingsMenu::HandleBackClicked); }
	if (ApplyGraphicsButton) { ApplyGraphicsButton->OnClicked.AddDynamic(this, &UBackroomsSettingsMenu::ApplyGraphics); }

	if (MasterVolumeSlider) { MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UBackroomsSettingsMenu::OnMasterVolumeChanged); }
	if (SFXVolumeSlider) { SFXVolumeSlider->OnValueChanged.AddDynamic(this, &UBackroomsSettingsMenu::OnSFXVolumeChanged); }
	if (AmbienceVolumeSlider) { AmbienceVolumeSlider->OnValueChanged.AddDynamic(this, &UBackroomsSettingsMenu::OnAmbienceVolumeChanged); }
	if (EntityVolumeSlider) { EntityVolumeSlider->OnValueChanged.AddDynamic(this, &UBackroomsSettingsMenu::OnEntityVolumeChanged); }

	if (ResolutionScaleSlider) { ResolutionScaleSlider->OnValueChanged.AddDynamic(this, &UBackroomsSettingsMenu::OnResolutionScaleChanged); }
	if (MouseSensitivitySlider) { MouseSensitivitySlider->OnValueChanged.AddDynamic(this, &UBackroomsSettingsMenu::OnMouseSensitivityChanged); }
	if (InvertYCheckBox) { InvertYCheckBox->OnCheckStateChanged.AddDynamic(this, &UBackroomsSettingsMenu::OnInvertYChanged); }

	// Start 0.5s timer for live FPS counter
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FPSTimerHandle, this, &UBackroomsSettingsMenu::UpdateFPSCounter, 0.5f, true);
	}
}

void UBackroomsSettingsMenu::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FPSTimerHandle);
	}

	Super::NativeDestruct();
}

FReply UBackroomsSettingsMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Gamepad_Special_Right)
	{
		HandleBackClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UBackroomsSettingsMenu::LoadValuesFromSettings()
{
	CurrentSettings = UBackroomsSaveGame::LoadOrCreateSettings();
	if (CurrentSettings)
	{
		PendingMasterVolume = CurrentSettings->MasterVolume;
		PendingSFXVolume = CurrentSettings->SFXVolume;
		PendingAmbienceVolume = CurrentSettings->AmbienceVolume;
		PendingEntityVolume = CurrentSettings->EntityVolume;
		SelectedQualityLevel = CurrentSettings->OverallQualityLevel;
		PendingResolutionScale = CurrentSettings->ResolutionScale;
		PendingSensitivity = CurrentSettings->MouseSensitivity;
		bPendingInvertY = CurrentSettings->bInvertY;
	}

	if (MasterVolumeSlider) { MasterVolumeSlider->SetValue(PendingMasterVolume); }
	if (SFXVolumeSlider) { SFXVolumeSlider->SetValue(PendingSFXVolume); }
	if (AmbienceVolumeSlider) { AmbienceVolumeSlider->SetValue(PendingAmbienceVolume); }
	if (EntityVolumeSlider) { EntityVolumeSlider->SetValue(PendingEntityVolume); }

	if (ResolutionScaleSlider) { ResolutionScaleSlider->SetValue(PendingResolutionScale); }
	if (MouseSensitivitySlider) { MouseSensitivitySlider->SetValue((PendingSensitivity - 0.1f) / 2.9f); }
	if (InvertYCheckBox) { InvertYCheckBox->SetIsChecked(bPendingInvertY); }

	if (MasterVolumeText) { MasterVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingMasterVolume * 100.0f)))); }
	if (SFXVolumeText) { SFXVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingSFXVolume * 100.0f)))); }
	if (AmbienceVolumeText) { AmbienceVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingAmbienceVolume * 100.0f)))); }
	if (EntityVolumeText) { EntityVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingEntityVolume * 100.0f)))); }
	if (ResolutionScaleText) { ResolutionScaleText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingResolutionScale)))); }
	if (MouseSensitivityText) { MouseSensitivityText->SetText(FText::FromString(FString::Printf(TEXT("%.1fx"), PendingSensitivity))); }
}

void UBackroomsSettingsMenu::UpdateFPSCounter()
{
	if (UWorld* World = GetWorld())
	{
		float DeltaSeconds = World->GetDeltaSeconds();
		float FPS = (DeltaSeconds > 0.0001f) ? (1.0f / DeltaSeconds) : 60.0f;
		int32 RoundedFPS = FMath::Clamp(FMath::RoundToInt(FPS), 0, 999);
		FString Str = FString::Printf(TEXT("FPS: %d"), RoundedFPS);

		FLinearColor FPSColor = (RoundedFPS >= 60) ? FLinearColor(111.0f / 255.0f, 168.0f / 255.0f, 90.0f / 255.0f, 1.0f)
		                      : ((RoundedFPS >= 30) ? FLinearColor(200.0f / 255.0f, 217.0f / 255.0f, 111.0f / 255.0f, 1.0f)
		                                           : FLinearColor(139.0f / 255.0f, 58.0f / 255.0f, 58.0f / 255.0f, 1.0f));

		if (FPSTextBlock)
		{
			FPSTextBlock->SetText(FText::FromString(Str));
			FPSTextBlock->SetColorAndOpacity(FSlateColor(FPSColor));
		}
		if (SlateFPSText.IsValid())
		{
			SlateFPSText->SetText(FText::FromString(Str));
			SlateFPSText->SetColorAndOpacity(FSlateColor(FPSColor));
		}
	}
}

void UBackroomsSettingsMenu::SwitchToTab(int32 TabIndex)
{
	PlayTactileClick();
	ActiveTabIndex = FMath::Clamp(TabIndex, 0, 2);

	UpdateTabVisuals();

	if (ContentSwitcher)
	{
		ContentSwitcher->SetActiveWidgetIndex(ActiveTabIndex);
	}
}

void UBackroomsSettingsMenu::UpdateTabVisuals()
{
	const FLinearColor ActiveBg(42.0f / 255.0f, 42.0f / 255.0f, 36.0f / 255.0f, 1.0f);
	const FLinearColor InactiveBg(20.0f / 255.0f, 20.0f / 255.0f, 18.0f / 255.0f, 1.0f);
	const FLinearColor ActiveText(200.0f / 255.0f, 217.0f / 255.0f, 111.0f / 255.0f, 1.0f);
	const FLinearColor InactiveText(122.0f / 255.0f, 122.0f / 255.0f, 116.0f / 255.0f, 1.0f);

	for (int32 i = 0; i < 3; ++i)
	{
		if (TabBorders[i].IsValid())
		{
			TabBorders[i]->SetBorderBackgroundColor((i == ActiveTabIndex) ? ActiveBg : InactiveBg);
		}
		if (TabTexts[i].IsValid())
		{
			TabTexts[i]->SetColorAndOpacity(FSlateColor((i == ActiveTabIndex) ? ActiveText : InactiveText));
		}
	}
}

void UBackroomsSettingsMenu::UpdatePresetVisuals()
{
	const FLinearColor ActiveBg(42.0f / 255.0f, 42.0f / 255.0f, 36.0f / 255.0f, 1.0f);
	const FLinearColor InactiveBg(20.0f / 255.0f, 20.0f / 255.0f, 18.0f / 255.0f, 1.0f);
	const FLinearColor ActiveText(200.0f / 255.0f, 217.0f / 255.0f, 111.0f / 255.0f, 1.0f);
	const FLinearColor InactiveText(122.0f / 255.0f, 122.0f / 255.0f, 116.0f / 255.0f, 1.0f);

	for (int32 i = 0; i < 4; ++i)
	{
		if (PresetBorders[i].IsValid())
		{
			PresetBorders[i]->SetBorderBackgroundColor((i == SelectedQualityLevel) ? ActiveBg : InactiveBg);
		}
		if (PresetTexts[i].IsValid())
		{
			PresetTexts[i]->SetColorAndOpacity(FSlateColor((i == SelectedQualityLevel) ? ActiveText : InactiveText));
		}
	}
}

void UBackroomsSettingsMenu::HandleBackClicked()
{
	PlayTactileClick();

	// Save and apply all settings on back
	if (CurrentSettings)
	{
		CurrentSettings->MasterVolume = PendingMasterVolume;
		CurrentSettings->SFXVolume = PendingSFXVolume;
		CurrentSettings->AmbienceVolume = PendingAmbienceVolume;
		CurrentSettings->EntityVolume = PendingEntityVolume;
		CurrentSettings->OverallQualityLevel = SelectedQualityLevel;
		CurrentSettings->ResolutionScale = PendingResolutionScale;
		CurrentSettings->MouseSensitivity = PendingSensitivity;
		CurrentSettings->bInvertY = bPendingInvertY;

		UBackroomsSaveGame::SaveSettings(CurrentSettings);
		UBackroomsSaveGame::ApplyAllSettings(this, CurrentSettings);
	}

	// Restore parent Pause Menu visibility and focus
	if (ParentPauseMenu.IsValid())
	{
		ParentPauseMenu->SetVisibility(ESlateVisibility::Visible);
		ParentPauseMenu->SetFocus();
	}

	RemoveFromParent();
}

void UBackroomsSettingsMenu::OnMasterVolumeChanged(float Value)
{
	PendingMasterVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	FString Str = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingMasterVolume * 100.0f));
	if (MasterVolumeText) { MasterVolumeText->SetText(FText::FromString(Str)); }
	if (SlateMasterText.IsValid()) { SlateMasterText->SetText(FText::FromString(Str)); }

	UBackroomsSaveGame::ApplyAudioSettings(this, PendingMasterVolume, PendingSFXVolume, PendingAmbienceVolume, PendingEntityVolume);
}

void UBackroomsSettingsMenu::OnSFXVolumeChanged(float Value)
{
	PendingSFXVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	FString Str = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingSFXVolume * 100.0f));
	if (SFXVolumeText) { SFXVolumeText->SetText(FText::FromString(Str)); }
	if (SlateSFXText.IsValid()) { SlateSFXText->SetText(FText::FromString(Str)); }

	UBackroomsSaveGame::ApplyAudioSettings(this, PendingMasterVolume, PendingSFXVolume, PendingAmbienceVolume, PendingEntityVolume);
}

void UBackroomsSettingsMenu::OnAmbienceVolumeChanged(float Value)
{
	PendingAmbienceVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	FString Str = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingAmbienceVolume * 100.0f));
	if (AmbienceVolumeText) { AmbienceVolumeText->SetText(FText::FromString(Str)); }
	if (SlateAmbienceText.IsValid()) { SlateAmbienceText->SetText(FText::FromString(Str)); }

	UBackroomsSaveGame::ApplyAudioSettings(this, PendingMasterVolume, PendingSFXVolume, PendingAmbienceVolume, PendingEntityVolume);
}

void UBackroomsSettingsMenu::OnEntityVolumeChanged(float Value)
{
	PendingEntityVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	FString Str = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingEntityVolume * 100.0f));
	if (EntityVolumeText) { EntityVolumeText->SetText(FText::FromString(Str)); }
	if (SlateEntityText.IsValid()) { SlateEntityText->SetText(FText::FromString(Str)); }

	UBackroomsSaveGame::ApplyAudioSettings(this, PendingMasterVolume, PendingSFXVolume, PendingAmbienceVolume, PendingEntityVolume);
}

void UBackroomsSettingsMenu::OnResolutionScaleChanged(float Value)
{
	PendingResolutionScale = FMath::Clamp(Value, 50.0f, 100.0f);
	FString Str = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingResolutionScale));
	if (ResolutionScaleText) { ResolutionScaleText->SetText(FText::FromString(Str)); }
	if (SlateResolutionText.IsValid()) { SlateResolutionText->SetText(FText::FromString(Str)); }
}

void UBackroomsSettingsMenu::SetQualityPreset(int32 QualityLevel)
{
	PlayTactileClick();
	SelectedQualityLevel = FMath::Clamp(QualityLevel, 0, 3);
	UpdatePresetVisuals();
}

void UBackroomsSettingsMenu::ApplyGraphics()
{
	PlayTactileClick();
	UBackroomsSaveGame::ApplyGraphicsSettings(SelectedQualityLevel, PendingResolutionScale);
}

void UBackroomsSettingsMenu::OnMouseSensitivityChanged(float Value)
{
	PendingSensitivity = FMath::Clamp(Value, 0.1f, 3.0f);
	FString Str = FString::Printf(TEXT("%.1fx"), PendingSensitivity);
	if (MouseSensitivityText) { MouseSensitivityText->SetText(FText::FromString(Str)); }
	if (SlateSensitivityText.IsValid()) { SlateSensitivityText->SetText(FText::FromString(Str)); }

	UBackroomsSaveGame::ApplyControlsSettings(this, PendingSensitivity, bPendingInvertY);
}

void UBackroomsSettingsMenu::OnInvertYChanged(bool bIsChecked)
{
	PlayTactileClick();
	bPendingInvertY = bIsChecked;
	UBackroomsSaveGame::ApplyControlsSettings(this, PendingSensitivity, bPendingInvertY);
}

TSharedRef<SWidget> UBackroomsSettingsMenu::BuildSlateAudioPanel()
{
	const FLinearColor ColorTextPrimary(232.0f / 255.0f, 230.0f / 255.0f, 223.0f / 255.0f, 1.0f);
	const FLinearColor ColorAccent(200.0f / 255.0f, 217.0f / 255.0f, 111.0f / 255.0f, 1.0f);
	const FLinearColor ColorBadgeBg(26.0f / 255.0f, 26.0f / 255.0f, 24.0f / 255.0f, 1.0f);
	const FLinearColor ColorBadgeBorder(58.0f / 255.0f, 58.0f / 255.0f, 52.0f / 255.0f, 1.0f);
	const FLinearColor ColorHelperText(138.0f / 255.0f, 138.0f / 255.0f, 122.0f / 255.0f, 0.85f);

	auto BuildSliderRow = [&](const FString& Label, float InitialVal, auto ValueCallback, TSharedPtr<STextBlock>& OutTextBlock) -> TSharedRef<SWidget>
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.35f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Label))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
					.ColorAndOpacity(FSlateColor(ColorTextPrimary))
				]
				+ SHorizontalBox::Slot().FillWidth(0.50f).VAlign(VAlign_Center).Padding(8.0f, 0.0f)
				[
					SNew(SSlider)
					.Value(InitialVal)
					.SliderBarColor(FSlateColor(FLinearColor(200.0f / 255.0f, 217.0f / 255.0f, 111.0f / 255.0f, 0.85f)))
					.SliderHandleColor(FSlateColor(ColorTextPrimary))
					.OnValueChanged_Lambda(ValueCallback)
				]
				+ SHorizontalBox::Slot().FillWidth(0.15f).HAlign(HAlign_Right).VAlign(VAlign_Center)
				[
					SNew(SBorder)
					.BorderBackgroundColor(ColorBadgeBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.Padding(FMargin(1.0f))
					[
						SNew(SBorder)
						.BorderBackgroundColor(ColorBadgeBg)
						.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.Padding(FMargin(8.0f, 3.0f))
						[
							SAssignNew(OutTextBlock, STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(InitialVal * 100.0f))))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
							.ColorAndOpacity(FSlateColor(ColorAccent))
						]
					]
				]
			];
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildSliderRow(TEXT("Master Volume"), PendingMasterVolume, [this](float V) { OnMasterVolumeChanged(V); }, SlateMasterText)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildSliderRow(TEXT("SFX Volume"), PendingSFXVolume, [this](float V) { OnSFXVolumeChanged(V); }, SlateSFXText)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildSliderRow(TEXT("Ambient Volume"), PendingAmbienceVolume, [this](float V) { OnAmbienceVolumeChanged(V); }, SlateAmbienceText)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildSliderRow(TEXT("AI Entity Volume"), PendingEntityVolume, [this](float V) { OnEntityVolumeChanged(V); }, SlateEntityText)
		]
		// Helper note
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("[!] Controls entity footsteps, stalking breathing, and chase cues.")))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 11))
			.ColorAndOpacity(FSlateColor(ColorHelperText))
		];
}

TSharedRef<SWidget> UBackroomsSettingsMenu::BuildSlateGraphicsPanel()
{
	const FLinearColor ColorTextPrimary(232.0f / 255.0f, 230.0f / 255.0f, 223.0f / 255.0f, 1.0f);
	const FLinearColor ColorAccent(200.0f / 255.0f, 217.0f / 255.0f, 111.0f / 255.0f, 1.0f);
	const FLinearColor ColorBadgeBg(26.0f / 255.0f, 26.0f / 255.0f, 24.0f / 255.0f, 1.0f);
	const FLinearColor ColorBadgeBorder(58.0f / 255.0f, 58.0f / 255.0f, 52.0f / 255.0f, 1.0f);

	const TCHAR* PresetNames[] = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") };

	TSharedRef<SHorizontalBox> SegmentedPresets = SNew(SHorizontalBox);
	for (int32 i = 0; i < 4; ++i)
	{
		const bool bIsActive = (i == SelectedQualityLevel);
		SegmentedPresets->AddSlot()
			.FillWidth(1.0f)
			.Padding(1.0f)
			[
				SAssignNew(PresetBorders[i], SBorder)
				.BorderBackgroundColor(bIsActive ? FLinearColor(42.0f / 255.0f, 42.0f / 255.0f, 36.0f / 255.0f, 1.0f)
				                                 : FLinearColor(20.0f / 255.0f, 20.0f / 255.0f, 18.0f / 255.0f, 1.0f))
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.Padding(FMargin(0.0f))
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked_Lambda([this, i]() {
						SetQualityPreset(i);
						return FReply::Handled();
					})
					[
						SAssignNew(PresetTexts[i], STextBlock)
						.Text(FText::FromString(PresetNames[i]))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						.ColorAndOpacity(FSlateColor(bIsActive ? ColorAccent : FLinearColor(122.0f / 255.0f, 122.0f / 255.0f, 116.0f / 255.0f, 1.0f)))
					]
				]
			];
	}

	return SNew(SVerticalBox)
		// Quality Preset
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.35f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Quality Preset")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				.ColorAndOpacity(FSlateColor(ColorTextPrimary))
			]
			+ SHorizontalBox::Slot().FillWidth(0.65f)
			[
				SegmentedPresets
			]
		]
		// Resolution Scale
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.35f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Resolution Scale")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				.ColorAndOpacity(FSlateColor(ColorTextPrimary))
			]
			+ SHorizontalBox::Slot().FillWidth(0.50f).VAlign(VAlign_Center).Padding(8.0f, 0.0f)
			[
				SNew(SSlider)
				.Value((PendingResolutionScale - 50.0f) / 50.0f)
				.SliderBarColor(FSlateColor(FLinearColor(140.0f / 255.0f, 140.0f / 255.0f, 130.0f / 255.0f, 0.85f)))
				.SliderHandleColor(FSlateColor(ColorTextPrimary))
				.OnValueChanged_Lambda([this](float V) { OnResolutionScaleChanged(50.0f + V * 50.0f); })
			]
			+ SHorizontalBox::Slot().FillWidth(0.15f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderBackgroundColor(ColorBadgeBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.Padding(FMargin(1.0f))
				[
					SNew(SBorder)
					.BorderBackgroundColor(ColorBadgeBg)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.Padding(FMargin(8.0f, 3.0f))
					[
						SAssignNew(SlateResolutionText, STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingResolutionScale))))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
						.ColorAndOpacity(FSlateColor(ColorAccent))
					]
				]
			]
		]
		// Live FPS Badge and Apply Button Row
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5f).VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderBackgroundColor(ColorBadgeBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.Padding(FMargin(1.0f))
				[
					SNew(SBorder)
					.BorderBackgroundColor(ColorBadgeBg)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.Padding(FMargin(12.0f, 6.0f))
					[
						SAssignNew(SlateFPSText, STextBlock)
						.Text(FText::FromString(TEXT("FPS: --")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
						.ColorAndOpacity(FSlateColor(ColorAccent))
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(170.0f)
				.HeightOverride(38.0f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked_Lambda([this]() { ApplyGraphics(); return FReply::Handled(); })
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("APPLY GRAPHICS")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						.ColorAndOpacity(FSlateColor(ColorTextPrimary))
					]
				]
			]
		];
}

TSharedRef<SWidget> UBackroomsSettingsMenu::BuildSlateControlsPanel()
{
	const FLinearColor ColorTextPrimary(232.0f / 255.0f, 230.0f / 255.0f, 223.0f / 255.0f, 1.0f);
	const FLinearColor ColorAccent(200.0f / 255.0f, 217.0f / 255.0f, 111.0f / 255.0f, 1.0f);
	const FLinearColor ColorBadgeBg(26.0f / 255.0f, 26.0f / 255.0f, 24.0f / 255.0f, 1.0f);
	const FLinearColor ColorBadgeBorder(58.0f / 255.0f, 58.0f / 255.0f, 52.0f / 255.0f, 1.0f);

	auto BuildKeyRow = [](const FString& Action, const FString& KeyName, bool bZebra) -> TSharedRef<SWidget>
	{
		const FLinearColor RowBg = bZebra ? FLinearColor(26.0f / 255.0f, 26.0f / 255.0f, 24.0f / 255.0f, 0.95f)
		                                  : FLinearColor(18.0f / 255.0f, 18.0f / 255.0f, 16.0f / 255.0f, 0.95f);

		return SNew(SBorder)
			.BorderBackgroundColor(RowBg)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.Padding(FMargin(12.0f, 5.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.55f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Action))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					.ColorAndOpacity(FSlateColor(FLinearColor(168.0f / 255.0f, 168.0f / 255.0f, 160.0f / 255.0f, 1.0f)))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.45f)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(42.0f / 255.0f, 42.0f / 255.0f, 36.0f / 255.0f, 1.0f))
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.Padding(FMargin(8.0f, 2.0f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(KeyName))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
						.ColorAndOpacity(FSlateColor(FLinearColor(232.0f / 255.0f, 230.0f / 255.0f, 223.0f / 255.0f, 1.0f)))
					]
				]
			];
	};

	return SNew(SVerticalBox)
		// Mouse Sensitivity
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.35f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Mouse Sensitivity")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				.ColorAndOpacity(FSlateColor(ColorTextPrimary))
			]
			+ SHorizontalBox::Slot().FillWidth(0.50f).VAlign(VAlign_Center).Padding(8.0f, 0.0f)
			[
				SNew(SSlider)
				.Value((PendingSensitivity - 0.1f) / 2.9f)
				.SliderBarColor(FSlateColor(FLinearColor(200.0f / 255.0f, 217.0f / 255.0f, 111.0f / 255.0f, 0.85f)))
				.SliderHandleColor(FSlateColor(ColorTextPrimary))
				.OnValueChanged_Lambda([this](float V) { OnMouseSensitivityChanged(0.1f + V * 2.9f); })
			]
			+ SHorizontalBox::Slot().FillWidth(0.15f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderBackgroundColor(ColorBadgeBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.Padding(FMargin(1.0f))
				[
					SNew(SBorder)
					.BorderBackgroundColor(ColorBadgeBg)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.Padding(FMargin(8.0f, 3.0f))
					[
						SAssignNew(SlateSensitivityText, STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%.1fx"), PendingSensitivity)))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
						.ColorAndOpacity(FSlateColor(ColorAccent))
					]
				]
			]
		]
		// Invert Y Axis
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.35f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Invert Y Axis")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				.ColorAndOpacity(FSlateColor(ColorTextPrimary))
			]
			+ SHorizontalBox::Slot().FillWidth(0.65f).VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked(bPendingInvertY ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { OnInvertYChanged(S == ECheckBoxState::Checked); })
			]
		]
		// Key Bindings Reference Table
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Key Bindings (Reference):")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				.ColorAndOpacity(FSlateColor(ColorAccent))
			]
			+ SVerticalBox::Slot().AutoHeight()[ BuildKeyRow(TEXT("Move Forward / Backward / Strafe"), TEXT("W  A  S  D"), false) ]
			+ SVerticalBox::Slot().AutoHeight()[ BuildKeyRow(TEXT("Look / Turn Camera"), TEXT("MOUSE DELTA"), true) ]
			+ SVerticalBox::Slot().AutoHeight()[ BuildKeyRow(TEXT("Sprint (Dynamic FOV 98°)"), TEXT("LEFT SHIFT"), false) ]
			+ SVerticalBox::Slot().AutoHeight()[ BuildKeyRow(TEXT("Jump"), TEXT("SPACE"), true) ]
			+ SVerticalBox::Slot().AutoHeight()[ BuildKeyRow(TEXT("Flashlight Toggle"), TEXT("F"), false) ]
			+ SVerticalBox::Slot().AutoHeight()[ BuildKeyRow(TEXT("Pause Menu / Resume"), TEXT("ESC / GAMEPAD START"), true) ]
		];
}

TSharedRef<SWidget> UBackroomsSettingsMenu::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	const FLinearColor ColorOverlay(10.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f, 0.96f);
	const FLinearColor ColorCardBg(26.0f / 255.0f, 26.0f / 255.0f, 24.0f / 255.0f, 0.99f);
	const FLinearColor ColorCardBorder(58.0f / 255.0f, 58.0f / 255.0f, 52.0f / 255.0f, 1.0f);
	const FLinearColor ColorAccent(200.0f / 255.0f, 217.0f / 255.0f, 111.0f / 255.0f, 1.0f);
	const FLinearColor ColorTextPrimary(232.0f / 255.0f, 230.0f / 255.0f, 223.0f / 255.0f, 1.0f);
	const FLinearColor ColorInactiveTab(122.0f / 255.0f, 122.0f / 255.0f, 116.0f / 255.0f, 1.0f);

	TSharedRef<SWidgetSwitcher> Switcher = SNew(SWidgetSwitcher)
		.WidgetIndex_Lambda([this]() { return ActiveTabIndex; })
		+ SWidgetSwitcher::Slot() [ BuildSlateAudioPanel() ]
		+ SWidgetSwitcher::Slot() [ BuildSlateGraphicsPanel() ]
		+ SWidgetSwitcher::Slot() [ BuildSlateControlsPanel() ];

	const TCHAR* TabNames[] = { TEXT("AUDIO"), TEXT("GRAPHICS"), TEXT("CONTROLS") };

	TSharedRef<SHorizontalBox> TabBar = SNew(SHorizontalBox);
	for (int32 i = 0; i < 3; ++i)
	{
		const bool bIsActive = (i == ActiveTabIndex);
		TabBar->AddSlot()
			.FillWidth(1.0f)
			.Padding(2.0f, 0.0f)
			[
				SAssignNew(TabBorders[i], SBorder)
				.BorderBackgroundColor(bIsActive ? FLinearColor(42.0f / 255.0f, 42.0f / 255.0f, 36.0f / 255.0f, 1.0f)
				                                 : FLinearColor(20.0f / 255.0f, 20.0f / 255.0f, 18.0f / 255.0f, 1.0f))
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.Padding(FMargin(0.0f))
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked_Lambda([this, i]() {
						SwitchToTab(i);
						return FReply::Handled();
					})
					[
						SAssignNew(TabTexts[i], STextBlock)
						.Text(FText::FromString(TabNames[i]))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
						.ColorAndOpacity(FSlateColor(bIsActive ? ColorAccent : ColorInactiveTab))
					]
				]
			];
	}

	TSharedRef<SWidget> ConstructedSlate =
		SNew(SOverlay)
		// Atmospheric solid dark overlay (completely blocks whatever was behind it)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(ColorOverlay)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		]
		// Centered 720px Card Box
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(720.0f)
			[
				// Outer Border
				SNew(SBorder)
				.BorderBackgroundColor(ColorCardBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.Padding(FMargin(2.0f))
				[
					// Card Body
					SNew(SBorder)
					.BorderBackgroundColor(ColorCardBg)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.Padding(FMargin(28.0f, 24.0f))
					[
						SNew(SVerticalBox)
						// Top accent strip
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 14.0f)
						[
							SNew(SBorder)
							.BorderBackgroundColor(ColorAccent)
							.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
							[
								SNew(SSpacer).Size(FVector2D(1.0f, 3.0f))
							]
						]
						// Title: SETTINGS
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("S E T T I N G S")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 32))
							.ColorAndOpacity(FSlateColor(ColorTextPrimary))
						]
						// Subtitle: Liminal terminal status
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.0f, 0.0f, 0.0f, 18.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("TERMINAL UTILITY // LEVEL 0 - PARAMETERS")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							.ColorAndOpacity(FSlateColor(ColorAccent))
						]
						// Tab Bar
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 18.0f)
						[
							TabBar
						]
						// Main Content Switcher
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(8.0f, 6.0f)
						[
							Switcher
						]
						// Back Button
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.0f, 24.0f, 0.0f, 0.0f)
						[
							SNew(SBox)
							.WidthOverride(220.0f)
							.HeightOverride(46.0f)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.OnClicked_Lambda([this]() {
									HandleBackClicked();
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("[ESC] BACK")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
									.ColorAndOpacity(FSlateColor(ColorTextPrimary))
								]
							]
						]
					]
				]
			]
		];

	return ConstructedSlate;
}
