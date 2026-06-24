// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_InventorySlotWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class UC_InventoryComponent;
class UDragDropOperation;

/**
 * 인벤토리 슬롯 한 칸 위젯 (C++ 베이스).
 * WBP_InventorySlot이 이 클래스를 부모로 사용 — 위젯 이름은 BindWidget으로 매칭.
 *   ItemIcon(UImage), QuantityText(UTextBlock)
 */
UCLASS()
class PROJECTBBK_API UC_InventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 슬롯 표시 갱신 — 아이템 있으면 아이콘+수량, 없으면 비움. Index = 격자 슬롯 위치.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSlot(UC_InventoryComponent* Inventory, FName ItemID, int32 Quantity, int32 Index);

	int32 GetSlotIndex() const { return slotIndex; }

protected:
	virtual void NativePreConstruct() override;

	// 드래그&드롭: 채워진 슬롯을 잡아 다른 슬롯으로 이동
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	UImage* SlotFrame = nullptr;

	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* QuantityText = nullptr;

	// 슬롯 테두리 텍스처 (WBP에서 T_InventorySlot 지정) — 코드에서 브러시 구성
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	UTexture2D* slotFrameTexture = nullptr;

private:
	TWeakObjectPtr<UC_InventoryComponent> inventoryComp;
	int32 slotIndex = -1;
	FName currentItemID;
	int32 currentQuantity = 0;   // 드래그 취소 시 원래 표시 복구용

	// 드래그 비주얼용 아이콘 캐시
	UPROPERTY()
	UTexture2D* currentIcon = nullptr;
};
