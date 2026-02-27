// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_MonsterHPWidgetBase.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_MonsterHPWidgetBase : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void SetMaxHp(float NewMaxHp);
	virtual void SetCurrentHp(float NewCurrentHp);
	virtual void SetMonsterName(const FText& NewName);

protected:
	float maxHp = 1.f;
	float currentHp = 1.f;
	
};
