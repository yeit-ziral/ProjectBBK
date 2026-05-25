// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LevelSequenceData.h"
#include "C_LoadingScreenWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;

UCLASS()
class PROJECTBBK_API UC_LoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// GameInstance에서 레벨 이동 직전 호출 — 텍스처/텍스트 설정
	UFUNCTION(BlueprintCallable, Category = "Loading")
	void InitializeLoadingScreen(const FLevelEntry& Entry);

	// FTSTicker에서 매 프레임 호출 — 0.0~1.0 범위
	void SetLoadingProgress(float Progress);

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* LoadingBackground;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* DescriptionText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TipText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UProgressBar* LoadingBar;
};
