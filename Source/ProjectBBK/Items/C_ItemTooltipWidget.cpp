// Fill out your copyright notice in the Description page of Project Settings.

#include "C_ItemTooltipWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

namespace
{
	// 0이 아닌 값만 "라벨 +값" 형식으로 줄 추가
	void AppendStatLine(TArray<FString>& lines, const FString& label, float value)
	{
		if (FMath::IsNearlyZero(value))
			return;

		const TCHAR* sign = value > 0.f ? TEXT("+") : TEXT("-");
		lines.Add(FString::Printf(TEXT("%s %s%g"), *label, sign, FMath::Abs(value)));
	}
}

void UC_ItemTooltipWidget::ApplyHeader(UTexture2D* icon, const FText& name)
{
	// 🟧 아이콘
	if (ItemIcon)
	{
		if (icon)
		{
			FSlateBrush brush;
			brush.SetResourceObject(icon);
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

	// 🟩 이름
	if (ItemNameText)
		ItemNameText->SetText(name);
}

void UC_ItemTooltipWidget::SetItem(const FEquipmentItemData& data)
{
	ApplyHeader(data.itemIcon, data.itemName);

	// 🟦 장비 종류
	if (SlotTypeText)
		SlotTypeText->SetText(GetSlotDisplayName(data.equipSlot));

	// 🟨 효과 목록 — 0이 아닌 스탯만 한 줄씩
	if (EffectsText)
	{
		TArray<FString> lines;
		AppendStatLine(lines, TEXT("공격력"),        data.bonusDamage);
		AppendStatLine(lines, TEXT("방어력"),        data.bonusDefense);
		AppendStatLine(lines, TEXT("최대 체력"),      data.bonusMaxHealth);
		AppendStatLine(lines, TEXT("최대 스태미나"),   data.bonusMaxStamina);
		AppendStatLine(lines, TEXT("이동 속도"),      data.bonusMoveSpeed);

		EffectsText->SetText(FText::FromString(FString::Join(lines, TEXT("\n"))));
	}
}

void UC_ItemTooltipWidget::SetConsumableItem(const FConsumableItemData& data)
{
	ApplyHeader(data.itemIcon, data.itemName);

	// 🟦 종류 — 소비 아이템 고정
	if (SlotTypeText)
		SlotTypeText->SetText(FText::FromString(TEXT("소비아이템")));

	// 🟨 설명 — DT의 description 원문을 그대로 표시 (consumeEffects 자동 조립 대신)
	if (EffectsText)
		EffectsText->SetText(data.description);
}

FText UC_ItemTooltipWidget::GetSlotDisplayName(EEquipmentSlot equipSlot)
{
	switch (equipSlot)
	{
	case EEquipmentSlot::Head:      return FText::FromString(TEXT("투구"));
	case EEquipmentSlot::Chest:     return FText::FromString(TEXT("갑옷"));
	case EEquipmentSlot::Legs:      return FText::FromString(TEXT("각반"));
	case EEquipmentSlot::Weapon:    return FText::FromString(TEXT("무기"));
	case EEquipmentSlot::Accessory: return FText::FromString(TEXT("장신구"));
	default:                        return FText::GetEmpty();
	}
}
