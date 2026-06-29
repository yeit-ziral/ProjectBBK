// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EquipmentSlotWidget.h"
#include "C_EquipmentComponent.h"
#include "../Inventory/C_InventorySlotWidget.h"
#include "../Items/ItemData.h"
#include "Components/Image.h"
#include "Blueprint/DragDropOperation.h"

void UC_EquipmentSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 슬롯 테두리 브러시 구성 (인벤토리 슬롯과 동일)
	if (SlotFrame && slotFrameTexture)
	{
		FSlateBrush brush;
		brush.SetResourceObject(slotFrameTexture);
		brush.DrawAs    = ESlateBrushDrawType::Image;
		brush.ImageSize = FVector2D(96.f, 96.f);
		SlotFrame->SetBrush(brush);
	}
}

void UC_EquipmentSlotWidget::RefreshSlot(UC_EquipmentComponent* Equip)
{
	equipComp = Equip;

	// 드롭 대상이 되도록 항상 hit-test 가능
	SetVisibility(ESlateVisibility::Visible);

	const FName itemID = Equip ? Equip->GetEquippedItem(slotType) : NAME_None;

	UTexture2D* icon = nullptr;
	if (Equip && !itemID.IsNone())
	{
		FBaseItemData data;
		if (Equip->GetItemData(itemID, data))
			icon = data.itemIcon;
	}
	currentIcon = icon;   // 드래그 비주얼용 캐시

	if (ItemIcon)
	{
		if (icon)
		{
			FSlateBrush brush;
			brush.SetResourceObject(icon);
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
}

FReply UC_EquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 장착돼 있을 때만 상호작용
	if (equipComp.IsValid() && equipComp->IsSlotEquipped(slotType))
	{
		// 우클릭 → 즉시 해제
		if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			equipComp->UnequipItem(slotType);   // OnEquipmentChanged → 장비창 갱신
			return FReply::Handled();
		}

		// 좌클릭 → 드래그 감지 시작(인벤토리로 끌어 해제). 안 끌면 더블클릭으로 처리됨
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UC_EquipmentSlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 좌클릭 더블클릭 → 인벤토리로 해제
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && equipComp.IsValid() && equipComp->IsSlotEquipped(slotType))
	{
		equipComp->UnequipItem(slotType);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UC_EquipmentSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (!equipComp.IsValid() || !equipComp->IsSlotEquipped(slotType))
		return;

	UDragDropOperation* Op = NewObject<UDragDropOperation>(GetTransientPackage(), UDragDropOperation::StaticClass());
	Op->Payload = this;                       // 드롭 시 출발 장비 슬롯 식별용
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

	// 드래그하는 동안 슬롯 아이콘 숨김(집어든 느낌, 커서 비주얼과 중복 방지)
	// 드롭 성공 시 OnEquipmentChanged로 갱신 / 취소 시 NativeOnDragCancelled에서 복구
	if (ItemIcon)
		ItemIcon->SetVisibility(ESlateVisibility::Hidden);

	OutOperation = Op;
}

void UC_EquipmentSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	// 인벤토리 밖 등 유효하지 않은 곳에 드롭 → 아직 장착 상태이므로 아이콘 복구
	RefreshSlot(equipComp.Get());
}

void UC_EquipmentSlotWidget::RequestUnequip()
{
	if (equipComp.IsValid() && equipComp->IsSlotEquipped(slotType))
		equipComp->UnequipItem(slotType);   // 가방으로 복귀 + OnEquipmentChanged
}

bool UC_EquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InOperation || !equipComp.IsValid())
		return false;

	// 인벤토리 슬롯에서 끌어온 것만 장착 대상
	UC_InventorySlotWidget* source = Cast<UC_InventorySlotWidget>(InOperation->Payload);
	if (!source)
		return false;

	const FName itemID = source->GetItemID();
	if (itemID.IsNone())
		return false;

	// 이 칸의 slotType과 드롭한 아이템의 장비 슬롯이 일치할 때만 장착
	// (예: 무기를 머리 칸에 드롭하면 거부). 장비가 아닌 소비 아이템(None)도 자동 거부.
	if (equipComp->GetItemSlotType(itemID) != slotType)
		return false;

	// 장착 (가방에서 제거 + GE 적용 → OnEquipmentChanged로 장비창 갱신). 미보유면 false
	return equipComp->EquipItem(itemID);
}
