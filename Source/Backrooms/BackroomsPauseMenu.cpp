// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsPauseMenu.h"
#include "BackroomsPlayerController.h"
#include "BackroomsSettingsMenu.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

UBackroomsPauseMenu::UBackroomsPauseMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
	DarkButtonStyle = CreateDarkButtonStyle();
}

FButtonStyle UBackroomsPauseMenu::CreateDarkButtonStyle() const
{
	FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");

	FSlateColorBrush NormalBrush(FLinearColor(40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 0.95f));
	FSlateColorBrush HoveredBrush(FLinearColor(65.0f / 255.0f, 65.0f / 255.0f, 65.0f / 255.0f, 1.0f));
	FSlateColorBrush PressedBrush(FLinearColor(20.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));

	Style.Normal.TintColor = FSlateColor(FLinearColor(40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 0.95f));
	Style.Hovered.TintColor = FSlateColor(FLinearColor(65.0f / 255.0f, 65.0f / 255.0f, 65.0f / 255.0f, 1.0f));
	Style.Pressed.TintColor = FSlateColor(FLinearColor(20.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f));

	return Style;
}

void UBackroomsPauseMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &UBackroomsPauseMenu::HandleResumeClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &UBackroomsPauseMenu::HandleSettingsClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UBackroomsPauseMenu::HandleQuitClicked);
	}
}

void UBackroomsPauseMenu::NativeDestruct()
{
	if (ResumeButton)
	{
		ResumeButton->OnClicked.RemoveDynamic(this, &UBackroomsPauseMenu::HandleResumeClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UBackroomsPauseMenu::HandleSettingsClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UBackroomsPauseMenu::HandleQuitClicked);
	}

	Super::NativeDestruct();
}

void UBackroomsPauseMenu::HandleResumeClicked()
{
	if (ABackroomsPlayerController* PC = Cast<ABackroomsPlayerController>(GetOwningPlayer()))
	{
		PC->ClosePauseMenu();
	}
}

void UBackroomsPauseMenu::HandleSettingsClicked()
{
	if (SettingsMenuInstance && SettingsMenuInstance->IsInViewport())
	{
		return;
	}

	TSubclassOf<UUserWidget> ClassToUse = SettingsMenuClass;
	if (!ClassToUse)
	{
		ClassToUse = UBackroomsSettingsMenu::StaticClass();
	}

	SettingsMenuInstance = CreateWidget<UUserWidget>(GetOwningPlayer(), ClassToUse);
	if (SettingsMenuInstance)
	{
		SettingsMenuInstance->AddToViewport(101); // In front of pause menu
	}
}

void UBackroomsPauseMenu::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

TSharedRef<SWidget> UBackroomsPauseMenu::RebuildWidget()
{
	// If a UMG designer widget hierarchy exists, use it
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	// Programmatic Slate Fallback
	TSharedRef<SWidget> ConstructedSlate =
		SNew(SOverlay)
		// Background semi-transparent black overlay (70% opacity)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.70f))
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		]
		// Centered 400px Menu Box
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(420.0f)
			[
				SNew(SVerticalBox)
				// PAUSED Title (48pt)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 35.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("PAUSED")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 44))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.0f)))
				]
				// Resume Button
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 6.0f)
				[
					SNew(SBox)
					.HeightOverride(52.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ButtonStyle(&DarkButtonStyle)
						.OnClicked_Lambda([this]() -> FReply {
							HandleResumeClicked();
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("RESUME")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
							.ColorAndOpacity(FSlateColor(FLinearColor::White))
						]
					]
				]
				// Settings Button
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 6.0f)
				[
					SNew(SBox)
					.HeightOverride(52.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ButtonStyle(&DarkButtonStyle)
						.OnClicked_Lambda([this]() -> FReply {
							HandleSettingsClicked();
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("SETTINGS")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
							.ColorAndOpacity(FSlateColor(FLinearColor::White))
						]
					]
				]
				// Quit to Desktop Button
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 6.0f)
				[
					SNew(SBox)
					.HeightOverride(52.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ButtonStyle(&DarkButtonStyle)
						.OnClicked_Lambda([this]() -> FReply {
							HandleQuitClicked();
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("QUIT TO DESKTOP")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
							.ColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.45f, 0.45f, 1.0f)))
						]
					]
				]
			]
		];

	return ConstructedSlate;
}
