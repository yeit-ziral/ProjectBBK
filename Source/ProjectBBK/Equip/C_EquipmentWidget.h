// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_EquipmentWidget.generated.h"

class UC_EquipmentComponent;
class UC_EquipmentSlotWidget;
class UCanvasPanel;

/**
 * 장비창 위젯 베이스 (캐릭터별 — meleeequip / rangedequip 배경).
 * WBP_EquipmentMelee / WBP_EquipmentRanged가 이 클래스를 부모로 사용.
 * 자식 트리의 모든 UC_EquipmentSlotWidget을 자동 수집 → 각 슬롯의 slotType에 따라 갱신.
 * 슬롯 배치/배경은 WBP 디자이너에서 그림에 맞춰 자유 배치.
 */
UCLASS()
class PROJECTBBK_API UC_EquipmentWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 현재 캐릭터의 장비 컴포넌트에 바인딩 — 캐릭터 교체 시 재호출(기존 바인딩 해제 후 재등록).
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetEquipment(UC_EquipmentComponent* Equip);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 창 이동 — 프레임(슬롯 외 배경)을 잡고 드래그하면 WindowRoot 위치 변경
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// OnEquipmentChanged 수신 → 전 슬롯 갱신
	UFUNCTION()
	void HandleEquipmentChanged();

	// 자식 트리에서 장비 슬롯 위젯을 모두 수집
	void CollectSlotWidgets();

	// 전 슬롯을 현재 장비 상태로 갱신
	void RefreshAllSlots();

	// 이동시킬 창 컨테이너 — WBP에서 RootCanvas 아래 "WindowRoot" CanvasPanel(배경+슬롯 포함)로 배치.
	// 없으면 창 이동 비활성(슬롯 동작은 정상).
	UPROPERTY(meta = (BindWidgetOptional))
	UCanvasPanel* WindowRoot = nullptr;

private:
	TWeakObjectPtr<UC_EquipmentComponent> equipComp;

	UPROPERTY(Transient)
	TArray<UC_EquipmentSlotWidget*> slotWidgets;

	// 창 드래그 상태
	bool bDraggingWindow = false;
	FVector2D dragOffset = FVector2D::ZeroVector;
};
