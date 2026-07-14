// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_ShopMessageWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * 상점 간단 안내 팝업 ("골드가 부족합니다" 등).
 * WBP_ShopMessage가 이 클래스를 부모로 사용.
 *   BindWidget: MessageText(UTextBlock)
 *   BindWidgetOptional: ConfirmButton(UButton) — 있으면 클릭 시 닫힘
 *
 * 버튼이 없어도 AutoCloseSeconds 후 스스로 사라진다.
 */
UCLASS()
class PROJECTBBK_API UC_ShopMessageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 안내 문구 표시 후 팝업을 띄운다(호출부에서 AddToViewport). 자동 닫힘 타이머 시작.
	void ShowMessage(const FText& Message);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MessageText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* ConfirmButton = nullptr;

	// 이 시간(초) 후 자동으로 닫힌다. 0 이하면 자동 닫힘 없음(버튼으로만 닫기).
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	float AutoCloseSeconds = 1.5f;

private:
	UFUNCTION()
	void OnConfirmClicked();

	void Close();

	FTimerHandle autoCloseTimer;
};
