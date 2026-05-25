// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EndingScreenWidget.h"
#include "C_BBKGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"

void UC_EndingScreenWidget::OnReturnToMainMenu()
{
	// StartGame()이 CurrentLevelIndex 리셋 + DA_LevelSequence Levels[0] 오픈을 함께 처리
	if (UC_BBKGameInstance* GI = Cast<UC_BBKGameInstance>(GetGameInstance()))
	{
		GI->StartGame();
	}
}

void UC_EndingScreenWidget::OnQuitGame()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}
