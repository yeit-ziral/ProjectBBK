// Fill out your copyright notice in the Description page of Project Settings.

#include "C_StatusWidget.h"
#include "../GAS/Attributes/C_ChracterAttributeSetBase.h"
#include "../Equip/C_EquipmentComponent.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "GameplayEffect.h"

namespace
{
	// 장비 보너스 합산에서 이 Attribute에 해당하는 필드를 뽑아옴 (5종 외에는 0)
	float GetEquipBonusForAttribute(const FEquipBonusTotals& Totals, const FGameplayAttribute& Attribute)
	{
		if (Attribute == UC_ChracterAttributeSetBase::GetmaxHealthAttribute())  return Totals.maxHealth;
		if (Attribute == UC_ChracterAttributeSetBase::GetmaxStaminaAttribute()) return Totals.maxStamina;
		if (Attribute == UC_ChracterAttributeSetBase::GetmoveSpeedAttribute())  return Totals.moveSpeed;
		if (Attribute == UC_ChracterAttributeSetBase::GetdefenseAttribute())    return Totals.defense;
		if (Attribute == UC_ChracterAttributeSetBase::GetdamageAttribute())     return Totals.damage;
		return 0.f;
	}
}

void UC_StatusWidget::InitializeStatWindow(UAbilitySystemComponent* ASC)
{
	// 기존 바인딩 해제 (캐릭터 교체 시 재호출 대비)
	if (cachedASC.IsValid())
	{
		cachedASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetmaxHealthAttribute()).Remove(maxHealthHandle);
		cachedASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetmaxStaminaAttribute()).Remove(maxStaminaHandle);
		cachedASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetmoveSpeedAttribute()).Remove(moveSpeedHandle);
		cachedASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetdefenseAttribute()).Remove(defenseHandle);
		cachedASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetdamageAttribute()).Remove(attackHandle);
	}

	cachedASC = ASC;

	if (!ASC)
		return;

	maxHealthHandle  = ASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetmaxHealthAttribute())
		.AddUObject(this, &UC_StatusWidget::OnMaxHealthChanged);
	maxStaminaHandle = ASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetmaxStaminaAttribute())
		.AddUObject(this, &UC_StatusWidget::OnMaxStaminaChanged);
	moveSpeedHandle  = ASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetmoveSpeedAttribute())
		.AddUObject(this, &UC_StatusWidget::OnMoveSpeedChanged);
	defenseHandle    = ASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetdefenseAttribute())
		.AddUObject(this, &UC_StatusWidget::OnDefenseChanged);
	attackHandle     = ASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetdamageAttribute())
		.AddUObject(this, &UC_StatusWidget::OnAttackChanged);

	RefreshStatValues();
}

void UC_StatusWidget::RefreshStatValues()
{
	if (!cachedASC.IsValid())
		return;

	UpdateStatText(MaxHPText,      cachedASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetmaxHealthAttribute()),  UC_ChracterAttributeSetBase::GetmaxHealthAttribute());
	UpdateStatText(MaxStaminaText, cachedASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetmaxStaminaAttribute()), UC_ChracterAttributeSetBase::GetmaxStaminaAttribute());
	UpdateStatText(MoveSpeedText,  cachedASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetmoveSpeedAttribute()),  UC_ChracterAttributeSetBase::GetmoveSpeedAttribute());
	UpdateStatText(DefenseText,    cachedASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetdefenseAttribute()),    UC_ChracterAttributeSetBase::GetdefenseAttribute());
	UpdateStatText(AttackText,     cachedASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetdamageAttribute()),     UC_ChracterAttributeSetBase::GetdamageAttribute());
}

void UC_StatusWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	UpdateStatText(MaxHPText, Data.NewValue, UC_ChracterAttributeSetBase::GetmaxHealthAttribute());
}

void UC_StatusWidget::OnMaxStaminaChanged(const FOnAttributeChangeData& Data)
{
	UpdateStatText(MaxStaminaText, Data.NewValue, UC_ChracterAttributeSetBase::GetmaxStaminaAttribute());
}

void UC_StatusWidget::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	UpdateStatText(MoveSpeedText, Data.NewValue, UC_ChracterAttributeSetBase::GetmoveSpeedAttribute());
}

void UC_StatusWidget::OnDefenseChanged(const FOnAttributeChangeData& Data)
{
	UpdateStatText(DefenseText, Data.NewValue, UC_ChracterAttributeSetBase::GetdefenseAttribute());
}

void UC_StatusWidget::OnAttackChanged(const FOnAttributeChangeData& Data)
{
	UpdateStatText(AttackText, Data.NewValue, UC_ChracterAttributeSetBase::GetdamageAttribute());
}

void UC_StatusWidget::UpdateStatText(UTextBlock* TextBlock, float NewValue, const FGameplayAttribute& Attribute) const
{
	if (!TextBlock)
		return;

	float bonus = GetPotionBonus(Attribute);

	if (cachedASC.IsValid())
	{
		if (const AActor* Avatar = cachedASC->GetAvatarActor())
		{
			if (const UC_EquipmentComponent* EquipComp = Avatar->FindComponentByClass<UC_EquipmentComponent>())
				bonus += GetEquipBonusForAttribute(EquipComp->GetTotalEquipBonuses(), Attribute);
		}
	}

	const int32 total = FMath::RoundToInt(NewValue);
	if (FMath::RoundToInt(bonus) != 0)
		TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d (+%d)"), total, FMath::RoundToInt(bonus))));
	else
		TextBlock->SetText(FText::AsNumber(total));
}

float UC_StatusWidget::GetPotionBonus(const FGameplayAttribute& Attribute) const
{
	if (!cachedASC.IsValid())
		return 0.f;

	static const FGameplayTag PotionBuffTag = FGameplayTag::RequestGameplayTag(FName("State.PotionBuff"));

	FGameplayEffectQuery query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(PotionBuffTag));
	const TArray<FActiveGameplayEffectHandle> handles = cachedASC->GetActiveEffects(query);

	float total = 0.f;
	for (const FActiveGameplayEffectHandle& handle : handles)
	{
		const FActiveGameplayEffect* activeGE = cachedASC->GetActiveGameplayEffect(handle);
		if (!activeGE || !activeGE->Spec.Def)
			continue;

		const TArray<FGameplayModifierInfo>& defModifiers = activeGE->Spec.Def->Modifiers;
		for (int32 i = 0; i < activeGE->Spec.Modifiers.Num(); ++i)
		{
			if (defModifiers.IsValidIndex(i) && defModifiers[i].Attribute == Attribute)
				total += activeGE->Spec.Modifiers[i].GetEvaluatedMagnitude();
		}
	}

	return total;
}

void UC_StatusWidget::NativeDestruct()
{
	if (cachedASC.IsValid())
	{
		cachedASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetmaxHealthAttribute()).Remove(maxHealthHandle);
		cachedASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetmaxStaminaAttribute()).Remove(maxStaminaHandle);
		cachedASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetmoveSpeedAttribute()).Remove(moveSpeedHandle);
		cachedASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetdefenseAttribute()).Remove(defenseHandle);
		cachedASC->GetGameplayAttributeValueChangeDelegate(UC_ChracterAttributeSetBase::GetdamageAttribute()).Remove(attackHandle);
	}

	Super::NativeDestruct();
}

FReply UC_StatusWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!WindowRoot || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	const FVector2D mouseAbs = InMouseEvent.GetScreenSpacePosition();

	if (DragHandle && !DragHandle->GetCachedGeometry().IsUnderLocation(mouseAbs))
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	const FVector2D localMouse = InGeometry.AbsoluteToLocal(mouseAbs);
	dragGrabOffset = localMouse - WindowRoot->GetRenderTransform().Translation;
	bIsDragging = true;

	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UC_StatusWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging && WindowRoot)
	{
		const FVector2D localMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		WindowRoot->SetRenderTranslation(localMouse - dragGrabOffset);
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UC_StatusWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
