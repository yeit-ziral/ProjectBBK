// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterHPWidgetBase.h"

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
