// Fill out your copyright notice in the Description page of Project Settings.

#include "C_InventorySlotWidget.h"
#include "C_InventoryComponent.h"
#include "../../../Items/ItemData.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

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

void UC_InventorySlotWidget::SetSlot(UC_InventoryComponent* Inventory, FName ItemID, int32 Quantity)
{
	FBaseItemData data;
	const bool bFound = Inventory && Inventory->GetItemData(ItemID, data);

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
