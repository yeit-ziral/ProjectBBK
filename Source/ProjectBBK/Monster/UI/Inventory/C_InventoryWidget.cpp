// Fill out your copyright notice in the Description page of Project Settings.

#include "C_InventoryWidget.h"
#include "C_InventorySlotWidget.h"
#include "C_InventoryComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"

void UC_InventoryWidget::SetInventory(UC_InventoryComponent* Inventory)
{
	// 기존 바인딩 해제 (재호출/캐릭터 교체 대비)
	if (inventoryComp.IsValid())
		inventoryComp->OnInventoryChanged.RemoveDynamic(this, &UC_InventoryWidget::OnInventoryChanged);

	inventoryComp = Inventory;

	if (inventoryComp.IsValid())
		inventoryComp->OnInventoryChanged.AddDynamic(this, &UC_InventoryWidget::OnInventoryChanged);

	RefreshInventory();
}

void UC_InventoryWidget::OnInventoryChanged()
{
	RefreshInventory();
}

void UC_InventoryWidget::RefreshInventory()
{
	if (!SlotGrid) return;

	SlotGrid->ClearChildren();

	if (!inventoryComp.IsValid() || !slotWidgetClass) return;

	const TArray<FInventorySlot> slots = inventoryComp->GetSlots();
	const int32 cols  = FMath::Max(1, columns);
	// 빈 칸 포함 격자 — 최소 gridSlotCount칸, 아이템이 더 많으면 그만큼 표시
	const int32 total = FMath::Max(gridSlotCount, slots.Num());

	for (int32 i = 0; i < total; ++i)
	{
		UC_InventorySlotWidget* slotWidget = CreateWidget<UC_InventorySlotWidget>(this, slotWidgetClass);
		if (!slotWidget) continue;

		if (i < slots.Num())
			slotWidget->SetSlot(inventoryComp.Get(), slots[i].itemID, slots[i].quantity);
		else
			slotWidget->SetSlot(inventoryComp.Get(), NAME_None, 0);   // 빈 칸(테두리만)

		SlotGrid->AddChildToUniformGrid(slotWidget, i / cols, i % cols);
	}
}

void UC_InventoryWidget::NativeDestruct()
{
	if (inventoryComp.IsValid())
		inventoryComp->OnInventoryChanged.RemoveDynamic(this, &UC_InventoryWidget::OnInventoryChanged);

	Super::NativeDestruct();
}
