// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsPauseMenu.h"
#include "BackroomsPlayerController.h"
#include "BackroomsSettingsMenu.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Sound/SoundBase.h"

UBackroomsPauseMenu::UBackroomsPauseMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
	LiminalButtonStyle = CreateLiminalButtonStyle(false);
	DangerButtonStyle = CreateLiminalButtonStyle(true);
}

FButtonStyle UBackroomsPauseMenu::CreateLiminalButtonStyle(bool bDanger) const
{
	FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");

	FLinearColor NormalCol = bDanger ? FLinearColor(42.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 0.95f)
	                                 : FLinearColor(42.0f / 255.0f, 42.0f / 255.0f, 36.0f / 255.0f, 0.95f);
	FLinearColor HoverCol  = bDanger ? FLinearColor(75.0f / 255.0f, 35.0f / 255.0f, 35.0f / 255.0f, 1.0f)
	                                 : FLinearColor(60.0f / 255.0f, 60.0f / 255.0f, 50.0f / 255.0f, 1.0f);
	FLinearColor PressedCol = bDanger ? FLinearColor(30.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f)
	                                  : FLinearColor(26.0f / 255.0f, 26.0f / 255.0f, 22.0f / 255.0f, 1.0f);

	Style.Normal.TintColor = FSlateColor(NormalCol);
	Style.Hovered.TintColor = FSlateColor(HoverCol);
	Style.Pressed.TintColor = FSlateColor(PressedCol);

	return Style;
}

void UBackroomsPauseMenu::PlayTactileClick()
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

FReply UBackroomsPauseMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Gamepad_Special_Right)
	{
		if (SettingsMenuInstance && SettingsMenuInstance->IsInViewport())
		{
			SettingsMenuInstance->RemoveFromParent();
			SettingsMenuInstance = nullptr;
			SetVisibility(ESlateVisibility::Visible);
			return FReply::Handled();
		}

		HandleResumeClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UBackroomsPauseMenu::HandleResumeClicked()
{
	PlayTactileClick();
	if (ABackroomsPlayerController* PC = Cast<ABackroomsPlayerController>(GetOwningPlayer()))
	{
		PC->ClosePauseMenu();
	}
}

void UBackroomsPauseMenu::HandleSettingsClicked()
{
	PlayTactileClick();

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
		if (UBackroomsSettingsMenu* Settings = Cast<UBackroomsSettingsMenu>(SettingsMenuInstance))
		{
			Settings->ParentPauseMenu = this;
		}

		// Completely hide the pause menu while settings are open so it doesn't ghost through!
		SetVisibility(ESlateVisibility::Hidden);
		SettingsMenuInstance->AddToViewport(101);
	}
}

void UBackroomsPauseMenu::HandleQuitClicked()
{
	PlayTactileClick();
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

TSharedRef<SWidget> UBackroomsPauseMenu::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	// Liminal horror colors
	const FLinearColor ColorOverlay(10.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f, 0.94f);
	const FLinearColor ColorCardBg(26.0f / 255.0f, 26.0f / 255.0f, 24.0f / 255.0f, 0.98f);
	const FLinearColor ColorCardBorder(58.0f / 255.0f, 58.0f / 255.0f, 52.0f / 255.0f, 1.0f);
	const FLinearColor ColorAccent(200.0f / 255.0f, 217.0f / 255.0f, 111.0f / 255.0f, 1.0f);
	const FLinearColor ColorTextPrimary(232.0f / 255.0f, 230.0f / 255.0f, 223.0f / 255.0f, 1.0f);
	const FLinearColor ColorDangerText(210.0f / 255.0f, 90.0f / 255.0f, 90.0f / 255.0f, 1.0f);
	const FLinearColor ColorFootnote(90.0f / 255.0f, 90.0f / 255.0f, 84.0f / 255.0f, 1.0f);

	TSharedRef<SWidget> ConstructedSlate =
		SNew(SOverlay)
		// Atmospheric background overlay (solid deep black to prevent ghosting)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(ColorOverlay)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		]
		// Centered 460px Menu Card
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(460.0f)
			[
				// Outer Border
				SNew(SBorder)
				.BorderBackgroundColor(ColorCardBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.Padding(FMargin(2.0f))
				[
					// Card Interior
					SNew(SBorder)
					.BorderBackgroundColor(ColorCardBg)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.Padding(FMargin(32.0f, 28.0f))
					[
						SNew(SVerticalBox)
						// Top accent strip
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 16.0f)
						[
							SNew(SBorder)
							.BorderBackgroundColor(ColorAccent)
							.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
							[
								SNew(SSpacer).Size(FVector2D(1.0f, 3.0f))
							]
						]
						// Title: PAUSED (monospaced terminal style)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.0f, 0.0f, 0.0f, 4.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("P A U S E D")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 36))
							.ColorAndOpacity(FSlateColor(ColorTextPrimary))
						]
						// Subtitle: Liminal status
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.0f, 0.0f, 0.0f, 24.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("- LEVEL 0 // SYSTEM STANDBY -")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
							.ColorAndOpacity(FSlateColor(ColorAccent))
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
								.ButtonStyle(&LiminalButtonStyle)
								.OnClicked_Lambda([this]() -> FReply {
									HandleResumeClicked();
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("RESUME")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
									.ColorAndOpacity(FSlateColor(ColorTextPrimary))
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
								.ButtonStyle(&LiminalButtonStyle)
								.OnClicked_Lambda([this]() -> FReply {
									HandleSettingsClicked();
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("SETTINGS")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
									.ColorAndOpacity(FSlateColor(ColorTextPrimary))
								]
							]
						]
						// Quit to Desktop Button
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 6.0f, 0.0f, 16.0f)
						[
							SNew(SBox)
							.HeightOverride(52.0f)
							[
								SNew(SButton)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.ButtonStyle(&DangerButtonStyle)
								.OnClicked_Lambda([this]() -> FReply {
									HandleQuitClicked();
									return FReply::Handled();
								})
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("QUIT TO DESKTOP")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
									.ColorAndOpacity(FSlateColor(ColorDangerText))
								]
							]
						]
						// Footer navigation hint
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("[ESC] RESUME    [ENTER] SELECT    [TAB] NAVIGATE")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
							.ColorAndOpacity(FSlateColor(ColorFootnote))
						]
					]
				]
			]
		];

	return ConstructedSlate;
}
