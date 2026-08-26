// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystemComponent.h"
#include "C_StatusWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * 플레이어 스탯 창 위젯 (C++ 베이스).
 * WBP_StatWindow가 이 클래스를 부모로 사용.
 * BindWidget: MaxHPText, MaxStaminaText, MoveSpeedText, DefenseText
 * BindWidgetOptional: WindowRoot (드래그 이동 대상), DragHandle (드래그 시작 영역)
 *
 * 각 텍스트는 "총합 (+증가분)" 형식으로 표시 — 증가분은 장비 착용(UC_EquipmentComponent)과
 * State.PotionBuff 태그가 부여된 포션(Duration GE)으로 인한 것만 반영. 스킬 버프/디버프는 제외.
 */
UCLASS()
class PROJECTBBK_API UC_StatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ASC 연결 → 어트리뷰트 변경 델리게이트 바인딩 + 현재 값으로 초기 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitializeStatWindow(UAbilitySystemComponent* ASC);

protected:
	virtual void NativeDestruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 드래그로 이동시킬 창 본체. WBP_StatWindow에서 창 컨테이너를 WindowRoot로 명명.
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* WindowRoot = nullptr;

	// 드래그 손잡이(선택). 지정 시 이 영역을 눌렀을 때만 이동. 미지정 시 창 전체에서 드래그 시작.
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* DragHandle = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MaxHPText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MaxStaminaText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MoveSpeedText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefenseText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AttackText = nullptr;

private:
	void RefreshStatValues();
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxStaminaChanged(const FOnAttributeChangeData& Data);
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
	void OnDefenseChanged(const FOnAttributeChangeData& Data);
	void OnAttackChanged(const FOnAttributeChangeData& Data);

	// NewValue(총합)에 장비+포션 증가분을 더해 "총합 (+증가분)" 텍스트를 만들어 반영. 증가분 0이면 괄호 생략.
	void UpdateStatText(UTextBlock* TextBlock, float NewValue, const FGameplayAttribute& Attribute) const;

	// State.PotionBuff 태그가 부여된 활성 GameplayEffect들의 Attribute Modifier 평가치 합산
	float GetPotionBonus(const FGameplayAttribute& Attribute) const;

	TWeakObjectPtr<UAbilitySystemComponent> cachedASC;

	FDelegateHandle maxHealthHandle;
	FDelegateHandle maxStaminaHandle;
	FDelegateHandle moveSpeedHandle;
	FDelegateHandle defenseHandle;
	FDelegateHandle attackHandle;

	bool bIsDragging = false;
	FVector2D dragGrabOffset = FVector2D::ZeroVector;
};
