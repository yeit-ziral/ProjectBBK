// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_MerchantDialogueWidget.generated.h"

class UTextBlock;
class UButton;
class UWidget;
class AC_MerchantNPC;
class AC_PlayerController;

/**
 * 상인 대화창 (화면 하단).
 * WBP_MerchantDialogue가 이 클래스를 부모로 사용.
 *   BindWidget: DialogueText(UTextBlock), BuyButton(UButton), LeaveButton(UButton)
 *   BindWidgetOptional: ChoicePanel(UWidget) — 구매/그만두기 버튼 묶음(대사 끝나기 전엔 숨김)
 *
 * 흐름: 대사를 한 줄씩 표시 → 창을 클릭하면 다음 줄 → 마지막 줄 뒤 구매/그만두기 노출.
 */
UCLASS()
class PROJECTBBK_API UC_MerchantDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 대화 시작 — NPC 대사 세팅 + 첫 줄 표시, 선택 버튼 숨김.
	void Setup(AC_MerchantNPC* NPC, AC_PlayerController* PC);

protected:
	virtual void NativeConstruct() override;

	// 창 클릭 → 다음 대사 줄로 진행 (마지막이면 선택지 노출)
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DialogueText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UButton* BuyButton = nullptr;

	UPROPERTY(meta = (BindWidget))
	UButton* LeaveButton = nullptr;

	// 구매/그만두기 버튼을 묶은 패널 — 대사 진행 중엔 Collapsed
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* ChoicePanel = nullptr;

private:
	UFUNCTION()
	void OnBuyClicked();

	UFUNCTION()
	void OnLeaveClicked();

	// 다음 줄로 진행. 마지막 줄이면 선택지를 노출하고 true 반환.
	void AdvanceDialogue();

	void ShowChoices();

	TWeakObjectPtr<AC_MerchantNPC> merchant;
	TWeakObjectPtr<AC_PlayerController> ownerPC;

	TArray<FText> lines;
	int32 currentLine = 0;
	bool bChoicesShown = false;
};
