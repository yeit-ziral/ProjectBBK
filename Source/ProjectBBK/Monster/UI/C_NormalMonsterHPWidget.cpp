// Fill out your copyright notice in the Description page of Project Settings.


#include "C_NormalMonsterHPWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"



void UC_NormalMonsterHPWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateWidget();
}

void UC_NormalMonsterHPWidget::UpdateWidget()
{
    if (LevelText)
    {
        LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv.%d"), monsterLevel)));
    }

    if (MonsterNameText)
    {
        if (bUseStatusText)
        {
            MonsterNameText->SetText(statusText);
        }
        else
        {
            MonsterNameText->SetText(monsterName);
        }
    }

    if (HpBar)
    {
        const float HpPercent = (maxHp > 0.0f) ? (curHp / maxHp) : 0.0f;
        HpBar->SetPercent(HpPercent);
    }

    if (HpText)
    {
        FString HpString = FString::Printf(TEXT("%.0f"), curHp);
        HpText->SetText(FText::FromString(HpString));
    }

    if (GroggyBar)
    {
        const float GroggyPercent = (maxGroggy > 0.0f) ? (curGroggy / maxGroggy) : 0.0f;
        GroggyBar->SetPercent(GroggyPercent);
    }
}


