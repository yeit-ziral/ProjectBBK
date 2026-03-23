// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossMonsterHPWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UC_BossMonsterHPWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateWidget();
}

void UC_BossMonsterHPWidget::UpdateWidget()
{
    if (BossLevelText)
    {
        BossLevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv.%d"), monsterLevel)));
    }

    if (BossNameText)
    {
        if (bUseStatusText)
        {
            BossNameText->SetText(statusText);
        }
        else
        {
            BossNameText->SetText(monsterName);
        }
    }

    if (BossHpBar)
    {
        const float HpPercent = (maxHp > 0.0f) ? (curHp / maxHp) : 0.0f;
        BossHpBar->SetPercent(HpPercent);
    }

    if (BossGroggyBar)
    {
        const float GroggyPercent = (maxGroggy > 0.0f) ? (curGroggy / maxGroggy) : 0.0f;
        BossGroggyBar->SetPercent(GroggyPercent);
    }
}
