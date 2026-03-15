// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_MonsterHPWidgetBase.h"
#include "C_NormalMonsterHPWidget.generated.h"


class UProgressBar;
class UTextBlock;
/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_NormalMonsterHPWidget : public UC_MonsterHPWidgetBase
{
	GENERATED_BODY()
	
public:


private:
    virtual void NativeConstruct() override;

    virtual void UpdateWidget() override;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* LevelText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* MonsterNameText;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* HpBar;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* GroggyBar;

};
