// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_QuantityPopupWidget.h"   // EShopTransactionMode
#include "C_ShopWidget.generated.h"

class UScrollBox;
class UTextBlock;
class UButton;
class UWidget;
class UC_ShopSlotWidget;
class UC_QuantityPopupWidget;
class UC_ShopMessageWidget;
class UC_InventoryComponent;
class AC_MerchantNPC;
class AC_PlayerController;

/**
 * 상점 판매창.
 * WBP_Shop이 이 클래스를 부모로 사용.
 *   BindWidget: StockList(UScrollBox), MoneyText(UTextBlock), CloseButton(UButton)
 *   EditDefaultsOnly: slotWidgetClass(WBP_ShopSlot), quantityPopupClass(WBP_QuantityPopup)
 *
 * - 상인 재고를 StockList에 채움. 슬롯 클릭 → 구매(수량>1이면 팝업).
 * - 내 인벤토리 슬롯을 이 창으로 드래그(또는 상점 열린 상태에서 인벤 슬롯 더블클릭) → 판매.
 * - 옆에 뜨는 "내 인벤토리"는 기존 WBP_Inventory를 그대로 사용(컨트롤러가 함께 열어줌).
 */
UCLASS()
class PROJECTBBK_API UC_ShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(AC_MerchantNPC* NPC, UC_InventoryComponent* Inventory, AC_PlayerController* PC);

	// 판매 목록 슬롯이 클릭됐을 때 (구매 요청).
	void OnStockSlotClicked(FName ItemID, int32 BuyPrice, int32 Stock);

	// 인벤토리 아이템 판매 요청 (드래그 드롭 / 더블클릭 진입점). AvailableQty = 보유 수량.
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestSellFromInventory(FName ItemID);

	// 수량 팝업 확정 콜백 (구매/판매 공용).
	void ConfirmQuantity(EShopTransactionMode Mode, FName ItemID, int32 Quantity);

	// 수량 팝업이 스스로 닫힐 때(확정/취소) 소유 상점에 알림 → activePopup 정리.
	void NotifyPopupClosed(UC_QuantityPopupWidget* Popup);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 인벤토리 슬롯을 이 창에 드롭 → 판매
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	// 창 드래그 이동 — WBP_Inventory와 동일한 방식(WindowRoot를 RenderTranslation으로 이동).
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 드래그로 이동시킬 대상(판매창 본체). WBP_Shop에서 창 컨테이너를 WindowRoot로 명명.
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* WindowRoot = nullptr;

	// 드래그 손잡이(선택) — 지정 시 이 영역을 눌렀을 때만 이동. 미지정 시 창 전체에서 드래그 시작.
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* DragHandle = nullptr;

	UPROPERTY(meta = (BindWidget))
	UScrollBox* StockList = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MoneyText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* CloseButton = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UC_ShopSlotWidget> slotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UC_QuantityPopupWidget> quantityPopupClass;

	// 안내 팝업(골드 부족 등). 미지정 시 안내창 없이 무동작.
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UC_ShopMessageWidget> messagePopupClass;

private:
	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnMoneyChanged(int32 NewAmount);

	void RefreshStock();
	void UpdateMoneyDisplay(int32 Amount);

	// 수량 팝업 생성(구매/판매 공용). 기존 팝업이 있으면 닫고 새로 띄운다(중복/교체 처리).
	void ShowQuantityPopup(EShopTransactionMode Mode, FName ItemID, int32 UnitPrice, int32 MaxQuantity);

	// 간단 안내 팝업 표시(골드 부족 등). 기존 안내창이 있으면 교체.
	void ShowMessage(const FText& Message);

	// 실제 구매/판매 처리 (수량 확정 후)
	void ExecuteBuy(FName ItemID, int32 Quantity);
	void ExecuteSell(FName ItemID, int32 Quantity);

	TWeakObjectPtr<AC_MerchantNPC> merchant;
	TWeakObjectPtr<UC_InventoryComponent> inventory;
	TWeakObjectPtr<AC_PlayerController> ownerPC;

	// 현재 떠 있는 수량 팝업(구매/판매). 동시에 하나만 유지한다.
	UPROPERTY()
	UC_QuantityPopupWidget* activePopup = nullptr;

	// 현재 떠 있는 안내 팝업(골드 부족 등). 동시에 하나만 유지한다.
	UPROPERTY()
	UC_ShopMessageWidget* activeMessage = nullptr;

	// 창 드래그 상태
	bool bIsDragging = false;
	FVector2D dragGrabOffset = FVector2D::ZeroVector;
};
