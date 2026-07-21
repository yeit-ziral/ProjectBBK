// Fill out your copyright notice in the Description page of Project Settings.

#include "C_UseItemSlotWidget.h"
#include "C_InventoryComponent.h"
#include "C_InventorySlotWidget.h"
#include "../Items/ItemData.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/DragDropOperation.h"

void UC_UseItemSlotWidget::SetSlotIndex(UC_InventoryComponent* Inventory, int32 Index)
{
	// 슬롯이 바뀌는 경우(재바인딩) 대비 기존 델리게이트 해제 후 재등록
	if (inventoryComp.IsValid())
		inventoryComp->OnQuickSlotChanged.RemoveDynamic(this, &UC_UseItemSlotWidget::OnQuickSlotChangedHandler);

	inventoryComp = Inventory;
	slotIndex     = Index;

	if (inventoryComp.IsValid())
		inventoryComp->OnQuickSlotChanged.AddDynamic(this, &UC_UseItemSlotWidget::OnQuickSlotChangedHandler);

	RefreshDisplay();
}

void UC_UseItemSlotWidget::NativeDestruct()
{
	if (inventoryComp.IsValid())
		inventoryComp->OnQuickSlotChanged.RemoveDynamic(this, &UC_UseItemSlotWidget::OnQuickSlotChangedHandler);

	Super::NativeDestruct();
}

void UC_UseItemSlotWidget::OnQuickSlotChangedHandler(int32 ChangedSlotIndex)
{
	if (ChangedSlotIndex == slotIndex)
		RefreshDisplay();
}

void UC_UseItemSlotWidget::RefreshDisplay()
{
	// 드롭 대상이 되도록 항상 hit-test 가능
	SetVisibility(ESlateVisibility::Visible);

	const FName itemID = inventoryComp.IsValid() ? inventoryComp->GetQuickSlotItem(slotIndex) : NAME_None;

	if (itemID.IsNone())
	{
		if (ItemIcon)     ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		if (QuantityText) QuantityText->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	FBaseItemData data;
	const bool bFound = inventoryComp->GetItemData(itemID, data);

	if (ItemIcon)
	{
		if (bFound && data.itemIcon)
		{
			FSlateBrush brush;
			brush.SetResourceObject(data.itemIcon);
			brush.DrawAs    = ESlateBrushDrawType::Image;
			brush.ImageSize = FVector2D(64.f, 64.f);
			ItemIcon->SetBrush(brush);
			ItemIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	const int32 count = inventoryComp->GetItemCount(itemID);
	const bool bOutOfStock = count <= 0;

	// 재고 0 — 등록은 유지한 채 아이콘만 반투명 처리 (자동 해제 안 함)
	if (ItemIcon)
		ItemIcon->SetRenderOpacity(bOutOfStock ? 0.35f : 1.0f);

	if (QuantityText)
	{
		if (!bOutOfStock && count > 1)
		{
			QuantityText->SetText(FText::AsNumber(count));
			QuantityText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			QuantityText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

bool UC_UseItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InOperation || !inventoryComp.IsValid())
		return false;

	// 인벤토리 슬롯에서 끌어온 것만 등록 대상 (장비 슬롯 등은 무시)
	UC_InventorySlotWidget* source = Cast<UC_InventorySlotWidget>(InOperation->Payload);
	if (!source)
		return false;

	const FName itemID = source->GetItemID();
	if (itemID.IsNone())
		return false;

	// 소비 아이템이 아니면 RegisterQuickSlot 내부에서 거부(false) — 인벤토리에서 제거되지 않음, 참조만 등록
	const bool bRegistered = inventoryComp->RegisterQuickSlot(slotIndex, itemID);

	// 아이템은 인벤토리에 그대로 남으므로, 드래그 시작 시 숨겼던 원본 슬롯의 아이콘/수량을 복구
	// (인벤토리 내부 이동과 달리 OnInventoryChanged가 브로드캐스트되지 않아 그리드가 재생성되지 않음)
	source->RestoreDisplay();

	return bRegistered;
}
