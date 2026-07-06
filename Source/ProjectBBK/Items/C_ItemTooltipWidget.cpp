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

	// 소비 효과 태그 → 표시용 한글 라벨. 미등록 태그는 마지막 세그먼트를 그대로 사용.
	FString ConsumableEffectLabel(const FGameplayTag& tag)
	{
		static const TMap<FName, FString> labels = {
			{ FName("Data.Heal"),    TEXT("체력 회복")     },
			{ FName("Data.Stamina"), TEXT("스태미나 회복") },
			{ FName("Data.Mana"),    TEXT("마나 회복")     },
			{ FName("Data.Exp"),     TEXT("경험치")        },
		};

		if (const FString* found = labels.Find(tag.GetTagName()))
			return *found;

		// 폴백: "Data.Heal" → "Heal"
		const FString full = tag.ToString();
		FString left, right;
		if (full.Split(TEXT("."), &left, &right, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
			return right;
		return full;
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

	// 🟨 효과 목록 — consumeEffects 각 항목(태그→라벨, magnitude→값)
	if (EffectsText)
	{
		TArray<FString> lines;
		for (const FConsumableEffectEntry& entry : data.consumeEffects)
			AppendStatLine(lines, ConsumableEffectLabel(entry.magnitudeTag), entry.magnitude);

		EffectsText->SetText(FText::FromString(FString::Join(lines, TEXT("\n"))));
	}
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
