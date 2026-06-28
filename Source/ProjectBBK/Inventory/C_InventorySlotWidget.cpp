// Fill out your copyright notice in the Description page of Project Settings.

#include "C_InventorySlotWidget.h"
#include "C_InventoryComponent.h"
#include "../Items/ItemData.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/DragDropOperation.h"

void UC_InventorySlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 슬롯 테두리 브러시를 코드에서 구성 (DrawAs/텍스처를 확실히 바인딩)
	if (SlotFrame && slotFrameTexture)
	{
		FSlateBrush brush;
		brush.SetResourceObject(slotFrameTexture);
		brush.DrawAs    = ESlateBrushDrawType::Image;
		brush.ImageSize = FVector2D(96.f, 96.f);
		SlotFrame->SetBrush(brush);
	}
}

void UC_InventorySlotWidget::SetSlot(UC_InventoryComponent* Inventory, FName ItemID, int32 Quantity, int32 Index)
{
	inventoryComp   = Inventory;
	slotIndex       = Index;
	currentItemID   = ItemID;
	currentQuantity = Quantity;
	currentIcon     = nullptr;

	// 빈 슬롯도 드롭 대상이 되도록 hit-test 가능하게
	SetVisibility(ESlateVisibility::Visible);

	FBaseItemData data;
	const bool bFound = Inventory && Inventory->GetItemData(ItemID, data);
	if (bFound)
		currentIcon = data.itemIcon;

	// 아이콘 — 브러시를 직접 구성(DrawAs=Image)해야 렌더됨
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

	// 수량 — 2개 이상일 때만 표시
	if (QuantityText)
	{
		if (Quantity > 1)
		{
			QuantityText->SetText(FText::AsNumber(Quantity));
			QuantityText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			QuantityText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

FReply UC_InventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 채워진 슬롯을 왼쪽 버튼으로 누르면 드래그 감지 시작
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !currentItemID.IsNone())
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UC_InventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UDragDropOperation* Op = NewObject<UDragDropOperation>(GetTransientPackage(), UDragDropOperation::StaticClass());
	Op->Payload = this;                       // 드롭 시 출발 슬롯 식별용
	Op->Pivot   = EDragPivot::CenterCenter;

	// 드래그 따라다니는 아이콘 비주얼
	if (currentIcon)
	{
		UImage* dragVisual = NewObject<UImage>(this);
		FSlateBrush brush;
		brush.SetResourceObject(currentIcon);
		brush.DrawAs    = ESlateBrushDrawType::Image;
		brush.ImageSize = FVector2D(64.f, 64.f);
		dragVisual->SetBrush(brush);
		Op->DefaultDragVisual = dragVisual;
	}

	// 드래그하는 동안 원래 슬롯의 아이콘/수량 숨김 (집어든 느낌)
	if (ItemIcon)     ItemIcon->SetVisibility(ESlateVisibility::Hidden);
	if (QuantityText) QuantityText->SetVisibility(ESlateVisibility::Hidden);

	OutOperation = Op;
}

void UC_InventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	// 유효하지 않은 곳에 드롭(취소) → 원래 표시 복구
	SetSlot(inventoryComp.Get(), currentItemID, currentQuantity, slotIndex);
}

bool UC_InventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InOperation) return false;

	UC_InventorySlotWidget* source = Cast<UC_InventorySlotWidget>(InOperation->Payload);
	if (!source || source == this || !inventoryComp.IsValid())
		return false;

	// Broadcast로 그리드가 즉시 재생성(this 파괴 가능)되므로 값을 먼저 캡처
	UC_InventoryComponent* comp = inventoryComp.Get();
	const int32 fromIdx = source->GetSlotIndex();
	const int32 toIdx   = slotIndex;

	comp->MoveSlot(fromIdx, toIdx);
	return true;
}
