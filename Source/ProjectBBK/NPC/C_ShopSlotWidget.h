// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_ShopSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UC_ShopWidget;
class UC_InventoryComponent;

/**
 * 상점 판매 목록의 한 칸 (상인이 파는 물건).
 * WBP_ShopSlot이 이 클래스를 부모로 사용.
 *   BindWidget: ItemIcon(UImage), NameText(UTextBlock), PriceText(UTextBlock)
 *
 * 클릭(또는 더블클릭) → 소유 상점 위젯에 구매 요청.
 */
UCLASS()
class PROJECTBBK_API UC_ShopSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 판매 항목 표시. Inventory는 아이콘/이름 조회용(GetItemData).
	void SetStock(UC_ShopWidget* Owner, UC_InventoryComponent* Inventory, FName ItemID, int32 BuyPrice, int32 Stock);

	FName GetItemID() const { return itemID; }

protected:
	// 좌클릭 → 구매(수량 1 또는 수량 팝업). 정책은 상점 위젯이 결정.
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* NameText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PriceText = nullptr;

private:
	TWeakObjectPtr<UC_ShopWidget> ownerShop;
	FName itemID = NAME_None;
	int32 buyPrice = 0;
	int32 stock = -1;
};
