// Fill out your copyright notice in the Description page of Project Settings.

#include "C_StatusWidget.h"
#include "../GAS/Attributes/C_ChracterAttributeSetBase.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

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

	if (MaxHPText)
		MaxHPText->SetText(FText::AsNumber(FMath::RoundToInt(cachedASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetmaxHealthAttribute()))));
	if (MaxStaminaText)
		MaxStaminaText->SetText(FText::AsNumber(FMath::RoundToInt(cachedASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetmaxStaminaAttribute()))));
	if (MoveSpeedText)
		MoveSpeedText->SetText(FText::AsNumber(FMath::RoundToInt(cachedASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetmoveSpeedAttribute()))));
	if (DefenseText)
		DefenseText->SetText(FText::AsNumber(FMath::RoundToInt(cachedASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetdefenseAttribute()))));
	if (AttackText)
		AttackText->SetText(FText::AsNumber(FMath::RoundToInt(cachedASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetdamageAttribute()))));
}

void UC_StatusWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	if (MaxHPText)
		MaxHPText->SetText(FText::AsNumber(FMath::RoundToInt(Data.NewValue)));
}

void UC_StatusWidget::OnMaxStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (MaxStaminaText)
		MaxStaminaText->SetText(FText::AsNumber(FMath::RoundToInt(Data.NewValue)));
}

void UC_StatusWidget::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (MoveSpeedText)
		MoveSpeedText->SetText(FText::AsNumber(FMath::RoundToInt(Data.NewValue)));
}

void UC_StatusWidget::OnDefenseChanged(const FOnAttributeChangeData& Data)
{
	if (DefenseText)
		DefenseText->SetText(FText::AsNumber(FMath::RoundToInt(Data.NewValue)));
}

void UC_StatusWidget::OnAttackChanged(const FOnAttributeChangeData& Data)
{
	if (AttackText)
		AttackText->SetText(FText::AsNumber(FMath::RoundToInt(Data.NewValue)));
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
