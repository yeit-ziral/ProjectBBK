// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_InventoryWidget.generated.h"

class UUniformGridPanel;
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

	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* SlotGrid = nullptr;

	// 슬롯 칸 위젯 클래스 — WBP_InventorySlot 지정
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UC_InventorySlotWidget> slotWidgetClass;

	// 한 줄에 표시할 칸 수
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 columns = 4;

	// 항상 표시할 빈 칸 포함 격자 칸 수 (아이템이 더 많으면 그만큼 늘어남)
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "0"))
	int32 gridSlotCount = 20;

private:
	UFUNCTION()
	void OnInventoryChanged();

	TWeakObjectPtr<UC_InventoryComponent> inventoryComp;
};
