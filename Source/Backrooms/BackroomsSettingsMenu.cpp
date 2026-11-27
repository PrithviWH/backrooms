// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsSettingsMenu.h"
#include "BackroomsSaveGame.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/GameUserSettings.h"
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

	FLinearColor NormalCol = bSelected ? FLinearColor(70.0f / 255.0f, 70.0f / 255.0f, 75.0f / 255.0f, 1.0f)
	                                   : FLinearColor(40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 0.95f);
	FLinearColor HoverCol = FLinearColor(65.0f / 255.0f, 65.0f / 255.0f, 65.0f / 255.0f, 1.0f);
	FLinearColor PressedCol = FLinearColor(20.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f);

	Style.Normal.TintColor = FSlateColor(NormalCol);
	Style.Hovered.TintColor = FSlateColor(HoverCol);
	Style.Pressed.TintColor = FSlateColor(PressedCol);

	return Style;
}

void UBackroomsSettingsMenu::NativeConstruct()
{
	Super::NativeConstruct();

	LoadValuesFromSettings();

	// Bind UMG events if present
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
	if (MasterVolumeText) { MasterVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingMasterVolume * 100.0f)))); }

	if (SFXVolumeSlider) { SFXVolumeSlider->SetValue(PendingSFXVolume); }
	if (SFXVolumeText) { SFXVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingSFXVolume * 100.0f)))); }

	if (AmbienceVolumeSlider) { AmbienceVolumeSlider->SetValue(PendingAmbienceVolume); }
	if (AmbienceVolumeText) { AmbienceVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingAmbienceVolume * 100.0f)))); }

	if (EntityVolumeSlider) { EntityVolumeSlider->SetValue(PendingEntityVolume); }
	if (EntityVolumeText) { EntityVolumeText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingEntityVolume * 100.0f)))); }

	if (ResolutionScaleSlider) { ResolutionScaleSlider->SetValue(PendingResolutionScale); }
	if (ResolutionScaleText) { ResolutionScaleText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingResolutionScale)))); }

	if (MouseSensitivitySlider) { MouseSensitivitySlider->SetValue(PendingSensitivity); }
	if (MouseSensitivityText) { MouseSensitivityText->SetText(FText::FromString(FString::Printf(TEXT("%.1fx"), PendingSensitivity))); }

	if (InvertYCheckBox) { InvertYCheckBox->SetIsChecked(bPendingInvertY); }
}

void UBackroomsSettingsMenu::UpdateFPSCounter()
{
	float FPS = 60.0f;
	if (GEngine)
	{
		extern ENGINE_API float GAverageFPS;
		FPS = GAverageFPS > 0.0f ? GAverageFPS : (1.0f / FMath::Max(GetWorld()->GetDeltaSeconds(), 0.0001f));
	}

	FString FPSSpec = FString::Printf(TEXT("FPS: %.0f"), FPS);
	if (FPSTextBlock)
	{
		FPSTextBlock->SetText(FText::FromString(FPSSpec));
	}
	if (SlateFPSText.IsValid())
	{
		SlateFPSText->SetText(FText::FromString(FPSSpec));
	}
}

void UBackroomsSettingsMenu::SwitchToTab(int32 TabIndex)
{
	ActiveTabIndex = FMath::Clamp(TabIndex, 0, 2);
	if (ContentSwitcher)
	{
		ContentSwitcher->SetActiveWidgetIndex(ActiveTabIndex);
	}
}

void UBackroomsSettingsMenu::HandleBackClicked()
{
	// Save current values to persistent settings
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
	SelectedQualityLevel = FMath::Clamp(QualityLevel, 0, 3);
}

void UBackroomsSettingsMenu::ApplyGraphics()
{
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
	bPendingInvertY = bIsChecked;
	UBackroomsSaveGame::ApplyControlsSettings(this, PendingSensitivity, bPendingInvertY);
}

TSharedRef<SWidget> UBackroomsSettingsMenu::BuildSlateAudioPanel()
{
	return SNew(SVerticalBox)
		// Master Volume
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("Master Volume"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)) ]
			+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center)
			[
				SNew(SSlider)
				.Value(PendingMasterVolume)
				.OnValueChanged_Lambda([this](float V) { OnMasterVolumeChanged(V); })
			]
			+ SHorizontalBox::Slot().FillWidth(0.15f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[ SAssignNew(SlateMasterText, STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingMasterVolume * 100.0f)))) ]
		]
		// SFX Volume
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("SFX Volume"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)) ]
			+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center)
			[
				SNew(SSlider)
				.Value(PendingSFXVolume)
				.OnValueChanged_Lambda([this](float V) { OnSFXVolumeChanged(V); })
			]
			+ SHorizontalBox::Slot().FillWidth(0.15f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[ SAssignNew(SlateSFXText, STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingSFXVolume * 100.0f)))) ]
		]
		// Ambience Volume
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("Ambient Volume"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)) ]
			+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center)
			[
				SNew(SSlider)
				.Value(PendingAmbienceVolume)
				.OnValueChanged_Lambda([this](float V) { OnAmbienceVolumeChanged(V); })
			]
			+ SHorizontalBox::Slot().FillWidth(0.15f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[ SAssignNew(SlateAmbienceText, STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingAmbienceVolume * 100.0f)))) ]
		]
		// AI Entity Volume
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("AI Entity Volume"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)) ]
			+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center)
			[
				SNew(SSlider)
				.Value(PendingEntityVolume)
				.OnValueChanged_Lambda([this](float V) { OnEntityVolumeChanged(V); })
			]
			+ SHorizontalBox::Slot().FillWidth(0.15f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[ SAssignNew(SlateEntityText, STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingEntityVolume * 100.0f)))) ]
		]
		// Note text
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Controls entity footsteps, stalking breathing, and chase cues.")))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 11))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f)))
		];
}

TSharedRef<SWidget> UBackroomsSettingsMenu::BuildSlateGraphicsPanel()
{
	return SNew(SVerticalBox)
		// Quality Preset
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.35f).VAlign(VAlign_Center)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("Quality Preset"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)) ]
			+ SHorizontalBox::Slot().FillWidth(0.65f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Low")))
					.OnClicked_Lambda([this]() { SetQualityPreset(0); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Medium")))
					.OnClicked_Lambda([this]() { SetQualityPreset(1); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("High")))
					.OnClicked_Lambda([this]() { SetQualityPreset(2); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Epic")))
					.OnClicked_Lambda([this]() { SetQualityPreset(3); return FReply::Handled(); })
				]
			]
		]
		// Resolution Scale
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("Resolution Scale"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)) ]
			+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center)
			[
				SNew(SSlider)
				.Value((PendingResolutionScale - 50.0f) / 50.0f)
				.OnValueChanged_Lambda([this](float V) { OnResolutionScaleChanged(50.0f + V * 50.0f); })
			]
			+ SHorizontalBox::Slot().FillWidth(0.15f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[ SAssignNew(SlateResolutionText, STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(PendingResolutionScale)))) ]
		]
		// Live FPS Counter and Apply Button
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 15.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.5f).VAlign(VAlign_Center)
			[
				SAssignNew(SlateFPSText, STextBlock)
				.Text(FText::FromString(TEXT("FPS: --")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.9f, 0.4f, 1.0f)))
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("APPLY GRAPHICS")))
				.OnClicked_Lambda([this]() { ApplyGraphics(); return FReply::Handled(); })
			]
		];
}

TSharedRef<SWidget> UBackroomsSettingsMenu::BuildSlateControlsPanel()
{
	return SNew(SVerticalBox)
		// Mouse Sensitivity
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("Mouse Sensitivity"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)) ]
			+ SHorizontalBox::Slot().FillWidth(0.45f).VAlign(VAlign_Center)
			[
				SNew(SSlider)
				.Value((PendingSensitivity - 0.1f) / 2.9f)
				.OnValueChanged_Lambda([this](float V) { OnMouseSensitivityChanged(0.1f + V * 2.9f); })
			]
			+ SHorizontalBox::Slot().FillWidth(0.15f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[ SAssignNew(SlateSensitivityText, STextBlock).Text(FText::FromString(FString::Printf(TEXT("%.1fx"), PendingSensitivity))) ]
		]
		// Invert Y Axis
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("Invert Y Axis"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)) ]
			+ SHorizontalBox::Slot().FillWidth(0.6f).VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked(bPendingInvertY ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { OnInvertYChanged(S == ECheckBoxState::Checked); })
			]
		]
		// Read-Only Key Bindings Reference
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 15.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("Key Bindings (Reference):"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 13)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("  Move: W A S D"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("  Look: Mouse"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("  Sprint: Left Shift"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("  Jump: Space"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f)
			[ SNew(STextBlock).Text(FText::FromString(TEXT("  Pause: ESC / Gamepad Start"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)) ]
		];
}

TSharedRef<SWidget> UBackroomsSettingsMenu::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	TSharedRef<SWidgetSwitcher> Switcher = SNew(SWidgetSwitcher)
		.WidgetIndex_Lambda([this]() { return ActiveTabIndex; })
		+ SWidgetSwitcher::Slot() [ BuildSlateAudioPanel() ]
		+ SWidgetSwitcher::Slot() [ BuildSlateGraphicsPanel() ]
		+ SWidgetSwitcher::Slot() [ BuildSlateControlsPanel() ];

	TSharedRef<SWidget> ConstructedSlate =
		SNew(SOverlay)
		// Background semi-transparent overlay (85% opacity)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f))
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		]
		// Centered 620px Menu Box
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(620.0f)
			[
				SNew(SVerticalBox)
				// SETTINGS Title (48pt)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 20.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("SETTINGS")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 40))
					.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
				// Tab Buttons: Audio, Graphics, Controls
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 20.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked_Lambda([this]() { SwitchToTab(0); return FReply::Handled(); })
						[ SNew(STextBlock).Text(FText::FromString(TEXT("AUDIO"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)) ]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked_Lambda([this]() { SwitchToTab(1); return FReply::Handled(); })
						[ SNew(STextBlock).Text(FText::FromString(TEXT("GRAPHICS"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)) ]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked_Lambda([this]() { SwitchToTab(2); return FReply::Handled(); })
						[ SNew(STextBlock).Text(FText::FromString(TEXT("CONTROLS"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)) ]
					]
				]
				// Content Area (Switcher)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10.0f, 10.0f)
				[
					Switcher
				]
				// Back Button
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 25.0f, 0.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(220.0f)
					.HeightOverride(46.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked_Lambda([this]() { HandleBackClicked(); return FReply::Handled(); })
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("BACK")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						]
					]
				]
			]
		];

	return ConstructedSlate;
}
