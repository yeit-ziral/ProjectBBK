// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_UseItemSlotWidget.generated.h"

class UImage;
class UTexture2D;
class UTextBlock;
class UC_InventoryComponent;
class UDragDropOperation;
class UMaterialInstanceDynamic;
class USoundBase;

/**
 * 퀵슬롯 한 칸 (F1/F2 등으로 즉시 사용하는 소비 아이템). WBP_UseItem이 이 클래스를 부모로 사용.
 * BindWidget: ItemIcon(UImage), QuantityText(UTextBlock, 선택), CooldownOverlay(UImage, 선택), CooldownText(UTextBlock, 선택).
 * 등록: 인벤토리 창이 열린 상태에서 UC_InventorySlotWidget을 드래그&드롭 — 인벤토리에서 제거하지 않고 itemID만 참조.
 * 사용: PlayerController의 IA_UseItem0/1 → UC_InventoryComponent::UseQuickSlot(slotIndex).
 * 쿨다운 오버레이: WBP_SkillIcon(C_SkillIconWidget)과 동일한 Progress 머티리얼 패턴 재사용 (Progress = 남은시간/전체쿨다운, 줄어드는 방향).
 */
UCLASS()
class PROJECTBBK_API UC_UseItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 칸이 담당하는 퀵슬롯 인덱스(0~1) — WBP_HUD에 배치 시 인스턴스마다 지정
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlot")
	void SetSlotIndex(UC_InventoryComponent* Inventory, int32 Index);

	// 등록된 아이템 아이콘/재고 상태 갱신 — OnQuickSlotChanged 바인딩 및 최초 초기화 시 호출
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlot")
	void RefreshDisplay();

	// 쿨다운 오버레이 갱신 — CurrentCooldown/MaxCooldown(초)
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlot")
	void UpdateCooldown(float CurrentCooldown, float MaxCooldown);

	// 쿨다운 오버레이 표시 여부
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlot")
	void SetCooldownVisible(bool bShow);

	// 재고 없음 알림 사운드 (2개 인스턴스 공용 사운드 하나로 충분)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|QuickSlot|Sound")
	USoundBase* outOfStockSound = nullptr;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	// 인벤토리 슬롯 드롭 → 소비 아이템이면 이 퀵슬롯에 등록
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UFUNCTION()
	void OnQuickSlotChangedHandler(int32 ChangedSlotIndex);

	UFUNCTION()
	void OnQuickSlotUseFailedHandler(int32 ChangedSlotIndex);

	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon = nullptr;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	UTextBlock* QuantityText = nullptr;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	UImage* CooldownOverlay = nullptr;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	UTextBlock* CooldownText = nullptr;

private:
	// SetSlotIndex 재호출(캐릭터 교체 등) 시 진행 중이던 쿨다운 상태를 오버레이에 복원
	void RestoreCooldownState();

	TWeakObjectPtr<UC_InventoryComponent> inventoryComp;
	int32 slotIndex = -1;

	UPROPERTY()
	UMaterialInstanceDynamic* cooldownMaterial = nullptr;

	float currentCooldownTime = 0.f;
	float maxCooldownTime = 0.f;
};
