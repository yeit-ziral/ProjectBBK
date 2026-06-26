// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_InventoryWidget.generated.h"

class UUniformGridPanel;
class UWidget;
class UTextBlock;
class UC_InventoryComponent;
class UC_InventorySlotWidget;

/**
 * 인벤토리 창 위젯 (C++ 베이스).
 * WBP_Inventory가 이 클래스를 부모로 사용 — BindWidget: SlotGrid(UUniformGridPanel).
 * SetInventory()로 컴포넌트를 연결하면 OnInventoryChanged에 바인딩 + 슬롯 갱신.
 */
UCLASS()
class PROJECTBBK_API UC_InventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 인벤토리 컴포넌트 연결 → 변경 델리게이트 바인딩 + 첫 갱신.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventory(UC_InventoryComponent* Inventory);

	// 슬롯 그리드를 현재 인벤토리 내용으로 다시 그림.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory();

protected:
	virtual void NativeDestruct() override;

	// 창 드래그 이동 — 손잡이(또는 창 전체)를 잡고 끌면 WindowRoot를 이동.
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* SlotGrid = nullptr;

	// 드래그로 이동시킬 대상(인벤토리 창 본체). WBP_Inventory에서 창 컨테이너를 WindowRoot로 명명.
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* WindowRoot = nullptr;

	// 드래그 손잡이(선택) — 지정 시 이 영역을 눌렀을 때만 이동. 미지정 시 창 전체에서 드래그 시작.
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* DragHandle = nullptr;

	// 보유 금액 표시 텍스트 (WBP에서 MoneyText로 명명) — 돈주머니 옆 숫자
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MoneyText = nullptr;

	// 슬롯 칸 위젯 클래스 — WBP_InventorySlot 지정
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UC_InventorySlotWidget> slotWidgetClass;

	// 한 줄에 표시할 칸 수
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 columns = 4;

private:
	UFUNCTION()
	void OnInventoryChanged();

	UFUNCTION()
	void OnMoneyChanged(int32 NewAmount);

	// MoneyText를 현재 금액으로 갱신
	void UpdateMoneyDisplay(int32 Amount);

	TWeakObjectPtr<UC_InventoryComponent> inventoryComp;

	// 드래그 상태
	bool bIsDragging = false;
	// 잡은 순간 WindowRoot 기준 마우스 로컬 위치 - 현재 변환량 (잡은 지점 유지용 offset)
	FVector2D dragGrabOffset = FVector2D::ZeroVector;
};
