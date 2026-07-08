// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_QuantityPopupWidget.generated.h"

class UTextBlock;
class UButton;
class UImage;
class UC_ShopWidget;
class UC_InventoryComponent;

// 거래 종류 — 구매 / 판매 공용
UENUM(BlueprintType)
enum class EShopTransactionMode : uint8
{
	Buy,
	Sell
};

/**
 * "몇 개 구매/판매 하시겠습니까?" 수량 선택 팝업.
 * WBP_QuantityPopup이 이 클래스를 부모로 사용.
 *   BindWidget: TitleText, QuantityText, TotalText(UTextBlock),
 *               PlusButton, MinusButton, ConfirmButton, CancelButton(UButton)
 *
 * 확정 시 소유 상점의 ConfirmQuantity를 호출한다.
 */
UCLASS()
class PROJECTBBK_API UC_QuantityPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Owner: 콜백 대상, Inventory: 아이콘/이름 조회용, Mode: 구매/판매, UnitPrice: 개당 가격, MaxQuantity: 상한(재고/보유량/소지금 기준)
	void Setup(UC_ShopWidget* Owner, UC_InventoryComponent* Inventory, EShopTransactionMode Mode, FName ItemID, int32 UnitPrice, int32 MaxQuantity);

protected:
	virtual void NativeConstruct() override;

	// 아이템 아이콘 (스샷의 빨간 네모)
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* ItemIcon = nullptr;

	// 아이템 이름 (스샷의 주황 네모 상단)
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* NameText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TitleText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* QuantityText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TotalText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UButton* PlusButton = nullptr;

	UPROPERTY(meta = (BindWidget))
	UButton* MinusButton = nullptr;

	UPROPERTY(meta = (BindWidget))
	UButton* ConfirmButton = nullptr;

	UPROPERTY(meta = (BindWidget))
	UButton* CancelButton = nullptr;

private:
	UFUNCTION() void OnPlusClicked();
	UFUNCTION() void OnMinusClicked();
	UFUNCTION() void OnConfirmClicked();
	UFUNCTION() void OnCancelClicked();

	void RefreshDisplay();

	TWeakObjectPtr<UC_ShopWidget> ownerShop;
	EShopTransactionMode mode = EShopTransactionMode::Buy;
	FName itemID = NAME_None;
	int32 unitPrice = 0;
	int32 maxQuantity = 1;
	int32 currentQuantity = 1;
};
