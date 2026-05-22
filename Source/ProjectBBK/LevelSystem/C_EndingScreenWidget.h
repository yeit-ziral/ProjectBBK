// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_EndingScreenWidget.generated.h"

UCLASS()
class PROJECTBBK_API UC_EndingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// BP 버튼에서 호출 — GameInstance 인덱스 초기화 후 메인 메뉴로 이동
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnReturnToMainMenu();

	// BP 버튼에서 호출 — 게임 종료
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnQuitGame();
};
