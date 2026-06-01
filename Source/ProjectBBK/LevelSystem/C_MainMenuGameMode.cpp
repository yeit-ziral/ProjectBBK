// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MainMenuGameMode.h"
#include "C_MainMenuWidget.h"
#include "Blueprint/UserWidget.h"

void AC_MainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!MainMenuWidgetClass) return;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	UC_MainMenuWidget* Widget = CreateWidget<UC_MainMenuWidget>(PC, MainMenuWidgetClass);
	if (Widget)
	{
		Widget->AddToViewport();
		PC->SetShowMouseCursor(true);
		PC->SetInputMode(FInputModeUIOnly());
	}
}
