// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterHPWidgetBase.h"

void UC_MonsterHPWidgetBase::SetMaxHp(float NewMaxHp)
{
	maxHp = FMath::Max(NewMaxHp, 1.f);
}

void UC_MonsterHPWidgetBase::SetCurrentHp(float NewCurrentHp)
{
	currentHp = FMath::Clamp(NewCurrentHp, 0.f, maxHp);
}

void UC_MonsterHPWidgetBase::SetMonsterName(const FText& NewName)
{
}
