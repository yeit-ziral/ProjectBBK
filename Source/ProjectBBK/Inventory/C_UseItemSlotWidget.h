// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_UseItemSlotWidget.generated.h"

class UImage;
class UTexture2D;
class UC_InventoryComponent;
class UDragDropOperation;

/**
 * 퀵슬롯 한 칸 (F1/F2 등으로 즉시 사용하는 소비 아이템). WBP_UseItem이 이 클래스를 부모로 사용.
 * BindWidget: ItemIcon(UImage), QuantityText(UTextBlock, 선택).
 * 등록: 인벤토리 창이 열린 상태에서 UC_InventorySlotWidget을 드래그&드롭 — 인벤토리에서 제거하지 않고 itemID만 참조.
 * 사용: PlayerController의 IA_UseItem0/1 → UC_InventoryComponent::UseQuickSlot(slotIndex).
 */
UCLASS()
class PROJECTBBK_API UC_UseItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 칸이 담당하는 퀵슬롯 인덱스(0~1) — WBP_HUD에 배치 시 인스턴스마다 지정
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlot")
	void SetSlotIndex(UC_InventoryComponent* Inventory, int32 Index);

	// 등록된 아이템 아이콘/재고 상태 갱신 — OnQuickSlotChanged 바인딩 및 최초 초기화 시 호출
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlot")
	void RefreshDisplay();

protected:
	virtual void NativeDestruct() override;

	// 인벤토리 슬롯 드롭 → 소비 아이템이면 이 퀵슬롯에 등록
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UFUNCTION()
	void OnQuickSlotChangedHandler(int32 ChangedSlotIndex);

	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon = nullptr;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	class UTextBlock* QuantityText = nullptr;

private:
	TWeakObjectPtr<UC_InventoryComponent> inventoryComp;
	int32 slotIndex = -1;
};
