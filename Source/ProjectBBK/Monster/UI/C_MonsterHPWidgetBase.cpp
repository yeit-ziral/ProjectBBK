// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterHPWidgetBase.h"
#include "Components/ProgressBar.h"

float UC_MonsterHPWidgetBase::MapFillPercent(float Ratio, float Start, float End)
{
	const float ClampedRatio = FMath::Clamp(Ratio, 0.0f, 1.0f);

	// 구간이 뒤집혔거나 비어 있으면 보정 없이 원래 비율을 그대로 쓴다.
	if (End <= Start)
		return ClampedRatio;

	return Start + ClampedRatio * (End - Start);
}

void UC_MonsterHPWidgetBase::UpdateGroggyBar(UProgressBar* Bar)
{
	if (!Bar) return;

	if (!bGroggyBarColorCached)
	{
		originalGroggyBarColor = Bar->GetFillColorAndOpacity();
		bGroggyBarColorCached  = true;
	}

	const float GroggyRatio = (maxGroggy > 0.0f) ? (curGroggy / maxGroggy) : 0.0f;
	Bar->SetPercent(MapFillPercent(GroggyRatio, groggyFillStart, groggyFillEnd));
	Bar->SetFillColorAndOpacity(bGroggyActive ? groggyActiveColor : originalGroggyBarColor);
}

void UC_MonsterHPWidgetBase::SetGroggyActive(bool bActive)
{
	if (bGroggyActive == bActive) return;

	bGroggyActive = bActive;
	UpdateWidget();
}

void UC_MonsterHPWidgetBase::SetMaxHp(float NewMaxHp)
{
	maxHp = FMath::Max(NewMaxHp, 1.0f);
	UpdateWidget();
}

void UC_MonsterHPWidgetBase::SetCurrentHp(float NewCurrentHp)
{
	curHp = FMath::Clamp(NewCurrentHp, 0.0f, maxHp);
	UpdateWidget();
}

void UC_MonsterHPWidgetBase::SetMaxGroggy(float NewMaxGroggy)
{
	maxGroggy = FMath::Max(NewMaxGroggy, 1.0f);
	UpdateWidget();
}

void UC_MonsterHPWidgetBase::SetCurrentGroggy(float NewCurrentGroggy)
{
	curGroggy = FMath::Clamp(NewCurrentGroggy, 0.0f, maxGroggy);
	UpdateWidget();
}

void UC_MonsterHPWidgetBase::SetMonsterLevel(int32 NewLevel)
{
	monsterLevel = NewLevel;
	UpdateWidget();
}

void UC_MonsterHPWidgetBase::SetMonsterName(const FText& NewName)
{
	monsterName = NewName;
	UpdateWidget();
}

void UC_MonsterHPWidgetBase::SetStatusText(const FText& NewStatusText)
{
	bUseStatusText = true;
	statusText = NewStatusText;
	UpdateWidget();
}

void UC_MonsterHPWidgetBase::ClearStatusText()
{
	bUseStatusText = false;
	statusText = FText::GetEmpty();
	UpdateWidget();
}
