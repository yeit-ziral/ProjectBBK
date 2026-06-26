// Fill out your copyright notice in the Description page of Project Settings.

#include "C_InventoryWidget.h"
#include "C_InventorySlotWidget.h"
#include "C_InventoryComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Widget.h"
#include "Components/TextBlock.h"

void UC_InventoryWidget::SetInventory(UC_InventoryComponent* Inventory)
{
	// 기존 바인딩 해제 (재호출/캐릭터 교체 대비)
	if (inventoryComp.IsValid())
	{
		inventoryComp->OnInventoryChanged.RemoveDynamic(this, &UC_InventoryWidget::OnInventoryChanged);
		inventoryComp->OnMoneyChanged.RemoveDynamic(this, &UC_InventoryWidget::OnMoneyChanged);
	}

	inventoryComp = Inventory;

	if (inventoryComp.IsValid())
	{
		inventoryComp->OnInventoryChanged.AddDynamic(this, &UC_InventoryWidget::OnInventoryChanged);
		inventoryComp->OnMoneyChanged.AddDynamic(this, &UC_InventoryWidget::OnMoneyChanged);
	}

	RefreshInventory();
	UpdateMoneyDisplay(inventoryComp.IsValid() ? inventoryComp->GetMoney() : 0);
}

void UC_InventoryWidget::OnMoneyChanged(int32 NewAmount)
{
	UpdateMoneyDisplay(NewAmount);
}

void UC_InventoryWidget::UpdateMoneyDisplay(int32 Amount)
{
	if (MoneyText)
		MoneyText->SetText(FText::AsNumber(Amount));
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

	// 고정 크기 슬롯 배열(빈칸 포함)을 격자 위치 그대로 표시 — 인덱스 = 자유 배치 위치
	const TArray<FInventorySlot> slots = inventoryComp->GetSlots();
	const int32 cols = FMath::Max(1, columns);

	for (int32 i = 0; i < slots.Num(); ++i)
	{
		UC_InventorySlotWidget* slotWidget = CreateWidget<UC_InventorySlotWidget>(this, slotWidgetClass);
		if (!slotWidget) continue;

		slotWidget->SetSlot(inventoryComp.Get(), slots[i].itemID, slots[i].quantity, i);

		SlotGrid->AddChildToUniformGrid(slotWidget, i / cols, i % cols);
	}
}

FReply UC_InventoryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!WindowRoot || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	const FVector2D mouseAbs = InMouseEvent.GetScreenSpacePosition();

	// 손잡이가 지정돼 있으면 그 위를 눌렀을 때만 드래그 시작
	if (DragHandle && !DragHandle->GetCachedGeometry().IsUnderLocation(mouseAbs))
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	// 잡은 지점이 창 안에서 고정되도록 offset 저장 (로컬 좌표 = 현재 translation 포함)
	const FVector2D localMouse = InGeometry.AbsoluteToLocal(mouseAbs);
	dragGrabOffset = localMouse - WindowRoot->GetRenderTransform().Translation;
	bIsDragging = true;

	// 마우스 캡처 → 창 밖으로 빠르게 끌어도 이동 유지
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UC_InventoryWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging && WindowRoot)
	{
		const FVector2D localMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		WindowRoot->SetRenderTranslation(localMouse - dragGrabOffset);
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UC_InventoryWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UC_InventoryWidget::NativeDestruct()
{
	if (inventoryComp.IsValid())
	{
		inventoryComp->OnInventoryChanged.RemoveDynamic(this, &UC_InventoryWidget::OnInventoryChanged);
		inventoryComp->OnMoneyChanged.RemoveDynamic(this, &UC_InventoryWidget::OnMoneyChanged);
	}

	Super::NativeDestruct();
}
