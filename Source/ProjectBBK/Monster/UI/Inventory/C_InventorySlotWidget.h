// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_InventorySlotWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class UC_InventoryComponent;

/**
 * 인벤토리 슬롯 한 칸 위젯 (C++ 베이스).
 * WBP_InventorySlot이 이 클래스를 부모로 사용 — 위젯 이름은 BindWidget으로 매칭.
 *   ItemIcon(UImage), QuantityText(UTextBlock)
 */
UCLASS()
class PROJECTBBK_API UC_InventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 슬롯 표시 갱신 — 아이템 있으면 아이콘+수량, 없으면 비움.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSlot(UC_InventoryComponent* Inventory, FName ItemID, int32 Quantity);

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UImage* SlotFrame = nullptr;

	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* QuantityText = nullptr;

	// 슬롯 테두리 텍스처 (WBP에서 T_InventorySlot 지정) — 코드에서 브러시 구성
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	UTexture2D* slotFrameTexture = nullptr;
};
