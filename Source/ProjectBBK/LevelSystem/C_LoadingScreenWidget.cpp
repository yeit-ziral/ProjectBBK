// Fill out your copyright notice in the Description page of Project Settings.

#include "C_LoadingScreenWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"

void UC_LoadingScreenWidget::InitializeLoadingScreen(const FLevelEntry& Entry)
{
	if (LoadingBackground)
	{
		if (UTexture2D* Tex = Entry.LoadingTexture.LoadSynchronous())
		{
			LoadingBackground->SetBrushFromTexture(Tex);
		}
	}

	if (DescriptionText)
		DescriptionText->SetText(Entry.LevelDescription);

	if (TipText)
		TipText->SetText(Entry.LoadingTip);

	if (LoadingBar)
		LoadingBar->SetPercent(0.f);
}

void UC_LoadingScreenWidget::SetLoadingProgress(float Progress)
{
	if (LoadingBar)
		LoadingBar->SetPercent(FMath::Clamp(Progress, 0.f, 1.f));
}
