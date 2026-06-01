// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EndingScreenWidget.h"
#include "C_BBKGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"

void UC_EndingScreenWidget::OnReturnToMainMenu()
{
	if (UC_BBKGameInstance* GI = Cast<UC_BBKGameInstance>(GetGameInstance()))
	{
		GI->TravelToMainMenu();
	}
}

void UC_EndingScreenWidget::OnQuitGame()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}
