// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Items/ItemData.h"   // EEquipmentSlot
#include "C_EquipmentSlotWidget.generated.h"

class UImage;
class UTexture2D;
class UC_EquipmentComponent;
class UC_ItemTooltipWidget;
class UDragDropOperation;

/**
 * 장비창의 슬롯 한 칸 (머리/무기/몸통 등). 비주얼은 인벤토리 슬롯과 동일 구성.
 * WBP_EquipmentSlot이 이 클래스를 부모로 사용 — BindWidget: SlotFrame(UImage), ItemIcon(UImage).
 *   slotType: 배치 시 이 칸이 어떤 EEquipmentSlot인지 지정.
 * 상호작용: 인벤토리 슬롯을 드롭 → 장착 / 우클릭·더블클릭 → 해제 / 드래그 → 인벤토리로 해제.
 */
UCLASS()
class PROJECTBBK_API UC_EquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 칸이 담당하는 장비 슬롯 — WBP 인스턴스마다 에디터에서 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	EEquipmentSlot slotType = EEquipmentSlot::None;

	// 현재 장착 상태를 반영해 아이콘 갱신 — 장비창 위젯이 호출
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RefreshSlot(UC_EquipmentComponent* Equip);

	// 이 슬롯의 장비를 해제(가방으로 복귀) — 인벤토리 슬롯에 드롭됐을 때 호출됨
	void RequestUnequip();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;

	// Slate 기본 툴팁(SetToolTip/Delegate)이 드래그형 창에서 미동작 → 직접 enter/leave로 뷰포트에 띄움
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	// 우클릭 → 해제 / 좌클릭 드래그 시작
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	// 좌클릭 더블클릭 → 해제
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	// 장착 아이콘을 인벤토리로 드래그
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	// 드래그 취소(유효하지 않은 곳에 드롭) → 숨겼던 아이콘 복구
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	// 인벤토리 슬롯 드롭 → 장착
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	UImage* SlotFrame = nullptr;

	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon = nullptr;

	// 슬롯 테두리 텍스처 (WBP에서 T_InventorySlot 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	UTexture2D* slotFrameTexture = nullptr;

	// 장착 아이템 hover 툴팁 (WBP에서 WBP_ItemTooltip 지정) — 미지정 시 툴팁 없음
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TSubclassOf<UC_ItemTooltipWidget> tooltipWidgetClass;

private:
	TWeakObjectPtr<UC_EquipmentComponent> equipComp;

	// 드래그 비주얼용 현재 아이콘 캐시 (RefreshSlot에서 갱신)
	UPROPERTY()
	UTexture2D* currentIcon = nullptr;

	// hover 중 표시 중인 툴팁 (없으면 null). MouseLeave/Destruct에서 정리.
	UPROPERTY()
	class UC_ItemTooltipWidget* activeTooltip = nullptr;
};
