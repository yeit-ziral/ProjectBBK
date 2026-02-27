// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_MonsterHPWidgetBase.h"
#include "C_BossMonsterHPWidget.generated.h"


class UProgressBar;
class UTextBlock;
/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_BossMonsterHPWidget : public UC_MonsterHPWidgetBase
{
	GENERATED_BODY()
public:
    virtual void SetMaxHp(float NewMaxHp) override;
    virtual void SetCurrentHp(float NewCurrentHp) override;
    virtual void SetMonsterName(const FText& NewName) override;

protected:
    virtual void NativeConstruct() override;

private:
    UPROPERTY(meta = (BindWidget))
    UProgressBar* BossHpBar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* BossNameText;
};
