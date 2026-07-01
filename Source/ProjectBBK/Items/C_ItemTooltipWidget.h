// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemData.h"   // FEquipmentItemData, EEquipmentSlot
#include "C_ItemTooltipWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * 장비 아이템 툴팁 — 인벤토리/장비 슬롯에 마우스를 올리면 표시.
 * WBP_ItemTooltip이 이 클래스를 부모로 사용 — BindWidget:
 *   ItemIcon(UImage)        🟧 아이템 이미지
 *   ItemNameText(UTextBlock) 🟩 아이템 이름
 *   SlotTypeText(UTextBlock) 🟦 장비 종류(무기/투구/갑옷...)
 *   EffectsText(UTextBlock)  🟨 효과 목록(공격력 +30 등, 멀티라인)
 * 슬롯 위젯이 SetItem()으로 내용을 채운 뒤 SetToolTip()으로 연결한다.
 */
UCLASS()
class PROJECTBBK_API UC_ItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 장비 데이터로 툴팁 내용 채우기 (아이콘/이름/슬롯종류/효과목록)
	UFUNCTION(BlueprintCallable, Category = "Tooltip")
	void SetItem(const FEquipmentItemData& data);

	// 소비 데이터로 툴팁 내용 채우기 (종류는 "소비아이템" 고정, 효과는 consumeEffects 기반)
	UFUNCTION(BlueprintCallable, Category = "Tooltip")
	void SetConsumableItem(const FConsumableItemData& data);

	// EEquipmentSlot → 표시용 한글 라벨
	UFUNCTION(BlueprintPure, Category = "Tooltip")
	static FText GetSlotDisplayName(EEquipmentSlot equipSlot);

private:
	// 아이콘 + 이름 공통 적용 (장비/소비 공유)
	void ApplyHeader(class UTexture2D* icon, const FText& name);

protected:
	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ItemNameText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SlotTypeText = nullptr;

	// 멀티라인 — 효과를 줄바꿈(\n)으로 이어붙여 한 텍스트로 표시
	UPROPERTY(meta = (BindWidget))
	UTextBlock* EffectsText = nullptr;
};
