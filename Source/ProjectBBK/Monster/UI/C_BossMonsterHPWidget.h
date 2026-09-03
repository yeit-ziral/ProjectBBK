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
    UC_BossMonsterHPWidget(const FObjectInitializer& ObjectInitializer);

    void SetInvincible(bool bInvincible);

private:
    virtual void NativeConstruct() override;
    virtual void UpdateWidget() override;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* BossHpBar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* BossNameText;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* BossGroggyBar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* BossLevelText;

    FLinearColor originalHpBarColor;
};
