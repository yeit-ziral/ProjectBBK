// Fill out your copyright notice in the Description page of Project Settings.

#include "C_ShopWidget.h"
#include "C_ShopSlotWidget.h"
#include "C_QuantityPopupWidget.h"
#include "C_MerchantNPC.h"
#include "../Inventory/C_InventoryComponent.h"
#include "../Inventory/C_InventorySlotWidget.h"
#include "../PlayerCharacter/PlayerAI/C_PlayerController.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Blueprint/DragDropOperation.h"

void UC_ShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton && !CloseButton->OnClicked.IsBound())
		CloseButton->OnClicked.AddDynamic(this, &UC_ShopWidget::OnCloseClicked);
}

void UC_ShopWidget::NativeDestruct()
{
	if (inventory.IsValid())
		inventory->OnMoneyChanged.RemoveDynamic(this, &UC_ShopWidget::OnMoneyChanged);

	Super::NativeDestruct();
}

void UC_ShopWidget::Setup(AC_MerchantNPC* NPC, UC_InventoryComponent* Inventory, AC_PlayerController* PC)
{
	// 기존 바인딩 해제 후 재연결 (재사용 대비)
	if (inventory.IsValid())
		inventory->OnMoneyChanged.RemoveDynamic(this, &UC_ShopWidget::OnMoneyChanged);

	merchant  = NPC;
	inventory = Inventory;
	ownerPC   = PC;

	if (Inventory)
	{
		Inventory->OnMoneyChanged.AddDynamic(this, &UC_ShopWidget::OnMoneyChanged);
		UpdateMoneyDisplay(Inventory->GetMoney());
	}

	RefreshStock();
}

void UC_ShopWidget::RefreshStock()
{
	if (!StockList || !slotWidgetClass || !merchant.IsValid())
		return;

	StockList->ClearChildren();

	const TArray<FMerchantStockEntry> Stock = merchant->GetStock();
	for (const FMerchantStockEntry& Entry : Stock)
	{
		UC_ShopSlotWidget* ShopSlot = CreateWidget<UC_ShopSlotWidget>(this, slotWidgetClass);
		if (!ShopSlot)
			continue;

		ShopSlot->SetStock(this, inventory.Get(), Entry.itemID, Entry.buyPrice, Entry.stock);
		StockList->AddChild(ShopSlot);
	}
}

void UC_ShopWidget::UpdateMoneyDisplay(int32 Amount)
{
	if (MoneyText)
		MoneyText->SetText(FText::Format(NSLOCTEXT("Shop", "MoneyFmt", "{0} G"), FText::AsNumber(Amount)));
}

void UC_ShopWidget::OnMoneyChanged(int32 NewAmount)
{
	UpdateMoneyDisplay(NewAmount);
}

// ---------- 구매 ----------

void UC_ShopWidget::OnStockSlotClicked(FName ItemID, int32 BuyPrice, int32 Stock)
{
	if (!inventory.IsValid() || ItemID.IsNone())
		return;

	const int32 Money = inventory->GetMoney();

	// 개당 가격이 0이면(설정 누락) 살 수 있는 수량 무한 → 안전하게 1개 처리
	const int32 affordable = (BuyPrice > 0) ? (Money / BuyPrice) : 1;
	const int32 stockCap   = (Stock < 0) ? affordable : Stock;
	const int32 maxQty     = FMath::Min(affordable, stockCap);

	if (maxQty <= 0)
		return; // 소지금/재고 부족 — 무동작 (BP에서 사운드 등 피드백 추가 가능)

	if (maxQty == 1 || !quantityPopupClass)
	{
		ExecuteBuy(ItemID, 1);
		return;
	}

	// 수량 팝업
	UC_QuantityPopupWidget* Popup = CreateWidget<UC_QuantityPopupWidget>(this, quantityPopupClass);
	if (!Popup)
	{
		ExecuteBuy(ItemID, 1);
		return;
	}
	Popup->Setup(this, inventory.Get(), EShopTransactionMode::Buy, ItemID, BuyPrice, maxQty);
	Popup->AddToViewport(20);
}

// ---------- 판매 ----------

void UC_ShopWidget::RequestSellFromInventory(FName ItemID)
{
	if (!inventory.IsValid() || !merchant.IsValid() || ItemID.IsNone())
		return;

	const int32 owned = inventory->GetItemCount(ItemID);
	if (owned <= 0)
		return;

	if (owned == 1 || !quantityPopupClass)
	{
		ExecuteSell(ItemID, 1);
		return;
	}

	const int32 sellPrice = merchant->GetSellPrice(ItemID);

	UC_QuantityPopupWidget* Popup = CreateWidget<UC_QuantityPopupWidget>(this, quantityPopupClass);
	if (!Popup)
	{
		ExecuteSell(ItemID, 1);
		return;
	}
	Popup->Setup(this, inventory.Get(), EShopTransactionMode::Sell, ItemID, sellPrice, owned);
	Popup->AddToViewport(20);
}

void UC_ShopWidget::ConfirmQuantity(EShopTransactionMode Mode, FName ItemID, int32 Quantity)
{
	if (Quantity <= 0)
		return;

	if (Mode == EShopTransactionMode::Buy)
		ExecuteBuy(ItemID, Quantity);
	else
		ExecuteSell(ItemID, Quantity);
}

void UC_ShopWidget::ExecuteBuy(FName ItemID, int32 Quantity)
{
	if (!inventory.IsValid() || !merchant.IsValid() || Quantity <= 0)
		return;

	const int32 unitPrice = merchant->GetBuyPrice(ItemID);
	const int32 total      = unitPrice * Quantity;

	// 소지금 확인 후 차감
	if (!inventory->RemoveMoney(total))
		return;

	// 아이템 지급 — 인벤토리가 가득 차 못 넣은 잔여분은 환불
	const int32 leftover = inventory->AddItem(ItemID, Quantity);
	if (leftover > 0)
		inventory->AddMoney(leftover * unitPrice);

	RefreshStock();
}

void UC_ShopWidget::ExecuteSell(FName ItemID, int32 Quantity)
{
	if (!inventory.IsValid() || !merchant.IsValid() || Quantity <= 0)
		return;

	const int32 unitPrice = merchant->GetSellPrice(ItemID);

	// 아이템 제거 성공 시에만 지급
	if (!inventory->RemoveItem(ItemID, Quantity))
		return;

	inventory->AddMoney(unitPrice * Quantity);

	RefreshStock();
}

bool UC_ShopWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InOperation)
		return false;

	UC_InventorySlotWidget* Source = Cast<UC_InventorySlotWidget>(InOperation->Payload);
	if (!Source)
		return false;

	const FName ItemID = Source->GetItemID();
	if (ItemID.IsNone())
		return false;

	// 드래그 중 숨겨진 원본 슬롯 아이콘을 복구 (판매가 팝업 취소로 무산돼도 시각 정상화)
	if (inventory.IsValid())
		inventory->OnInventoryChanged.Broadcast();

	RequestSellFromInventory(ItemID);
	return true;
}

void UC_ShopWidget::OnCloseClicked()
{
	if (ownerPC.IsValid())
		ownerPC->CloseShop();
}

// ---------- 창 드래그 이동 (WBP_Inventory와 동일) ----------

FReply UC_ShopWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!WindowRoot || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	const FVector2D mouseAbs = InMouseEvent.GetScreenSpacePosition();

	// 손잡이가 지정돼 있으면 그 위를 눌렀을 때만 드래그 시작
	if (DragHandle && !DragHandle->GetCachedGeometry().IsUnderLocation(mouseAbs))
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	const FVector2D localMouse = InGeometry.AbsoluteToLocal(mouseAbs);
	dragGrabOffset = localMouse - WindowRoot->GetRenderTransform().Translation;
	bIsDragging = true;

	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UC_ShopWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging && WindowRoot)
	{
		const FVector2D localMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		WindowRoot->SetRenderTranslation(localMouse - dragGrabOffset);
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UC_ShopWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
