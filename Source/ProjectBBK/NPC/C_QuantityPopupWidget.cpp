// Fill out your copyright notice in the Description page of Project Settings.

#include "C_QuantityPopupWidget.h"
#include "C_ShopWidget.h"
#include "../Inventory/C_InventoryComponent.h"
#include "../Items/ItemData.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"

void UC_QuantityPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlusButton && !PlusButton->OnClicked.IsBound())
		PlusButton->OnClicked.AddDynamic(this, &UC_QuantityPopupWidget::OnPlusClicked);
	if (MinusButton && !MinusButton->OnClicked.IsBound())
		MinusButton->OnClicked.AddDynamic(this, &UC_QuantityPopupWidget::OnMinusClicked);
	if (ConfirmButton && !ConfirmButton->OnClicked.IsBound())
		ConfirmButton->OnClicked.AddDynamic(this, &UC_QuantityPopupWidget::OnConfirmClicked);
	if (CancelButton && !CancelButton->OnClicked.IsBound())
		CancelButton->OnClicked.AddDynamic(this, &UC_QuantityPopupWidget::OnCancelClicked);
}

void UC_QuantityPopupWidget::Setup(UC_ShopWidget* Owner, UC_InventoryComponent* Inventory, EShopTransactionMode Mode, FName ItemID, int32 UnitPrice, int32 MaxQuantity)
{
	ownerShop   = Owner;
	mode        = Mode;
	itemID      = ItemID;
	unitPrice   = UnitPrice;
	maxQuantity = FMath::Max(1, MaxQuantity);
	currentQuantity = 1;

	if (TitleText)
	{
		TitleText->SetText(Mode == EShopTransactionMode::Buy
			? NSLOCTEXT("Shop", "BuyQtyTitle", "몇 개 구매하시겠습니까?")
			: NSLOCTEXT("Shop", "SellQtyTitle", "몇 개 판매하시겠습니까?"));
	}

	// 아이콘 / 이름 표시 (C_ShopSlotWidget과 동일한 방식으로 인벤토리에서 조회)
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

	RefreshDisplay();
}

void UC_QuantityPopupWidget::RefreshDisplay()
{
	if (QuantityText)
		QuantityText->SetText(FText::AsNumber(currentQuantity));

	if (TotalText)
	{
		TotalText->SetText(FText::Format(
			NSLOCTEXT("Shop", "TotalFmt", "최종 {0} G"),
			FText::AsNumber(unitPrice * currentQuantity)));
	}
}

void UC_QuantityPopupWidget::OnPlusClicked()
{
	currentQuantity = FMath::Min(currentQuantity + 1, maxQuantity);
	RefreshDisplay();
}

void UC_QuantityPopupWidget::OnMinusClicked()
{
	currentQuantity = FMath::Max(currentQuantity - 1, 1);
	RefreshDisplay();
}

void UC_QuantityPopupWidget::OnConfirmClicked()
{
	if (ownerShop.IsValid())
	{
		ownerShop->ConfirmQuantity(mode, itemID, currentQuantity);
		ownerShop->NotifyPopupClosed(this);
	}

	RemoveFromParent();
}

void UC_QuantityPopupWidget::OnCancelClicked()
{
	if (ownerShop.IsValid())
		ownerShop->NotifyPopupClosed(this);

	RemoveFromParent();
}
