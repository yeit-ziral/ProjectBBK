// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MainMenuWidget.h"
#include "C_BBKGameInstance.h"
#include "Components/Button.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/KismetSystemLibrary.h"

void UC_MainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StartButton)
		StartButton->OnClicked.AddDynamic(this, &UC_MainMenuWidget::OnStartClicked);
	if (SettingsButton)
		SettingsButton->OnClicked.AddDynamic(this, &UC_MainMenuWidget::OnSettingsClicked);
	if (QuitButton)
		QuitButton->OnClicked.AddDynamic(this, &UC_MainMenuWidget::OnQuitClicked);
}

void UC_MainMenuWidget::OnStartClicked()
{
	if (UC_BBKGameInstance* GI = Cast<UC_BBKGameInstance>(GetGameInstance()))
	{
		GI->StartGame();
	}
}

void UC_MainMenuWidget::OnSettingsClicked()
{
	if (!SettingsWidgetClass) return;

	UUserWidget* SettingsWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), SettingsWidgetClass);
	if (SettingsWidget)
	{
		SettingsWidget->AddToViewport(1);
	}
}

void UC_MainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
}
