// Fill out your copyright notice in the Description page of Project Settings.

#include "C_ShopSlotWidget.h"
#include "C_ShopWidget.h"
#include "../Inventory/C_InventoryComponent.h"
#include "../Items/ItemData.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UC_ShopSlotWidget::SetStock(UC_ShopWidget* Owner, UC_InventoryComponent* Inventory, FName ItemID, int32 BuyPrice, int32 Stock)
{
	ownerShop = Owner;
	itemID    = ItemID;
	buyPrice  = BuyPrice;
	stock     = Stock;

	FBaseItemData data;
	const bool bFound = Inventory && Inventory->GetItemData(ItemID, data);

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

	if (NameText)
		NameText->SetText(bFound ? data.itemName : FText::FromName(ItemID));

	if (PriceText)
		PriceText->SetText(FText::Format(NSLOCTEXT("Shop", "PriceFmt", "{0} G"), FText::AsNumber(BuyPrice)));
}

FReply UC_ShopSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !itemID.IsNone() && ownerShop.IsValid())
	{
		ownerShop->OnStockSlotClicked(itemID, buyPrice, stock);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
