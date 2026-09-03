// Fill out your copyright notice in the Description page of Project Settings.


#include "C_NormalMonsterHPWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"



void UC_NormalMonsterHPWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (MonsterNameText)
		MonsterNameText->SetJustification(ETextJustify::Center);
	UpdateWidget();
}

void UC_NormalMonsterHPWidget::UpdateWidget()
{
    if (LevelText)
    {
        LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv.%d"), monsterLevel)));
        LevelText->SetVisibility(bUseStatusText ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
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
        const float HpRatio = (maxHp > 0.0f) ? (curHp / maxHp) : 0.0f;
        HpBar->SetPercent(MapFillPercent(HpRatio, hpFillStart, hpFillEnd));
    }

    if (HpText)
    {
        FString HpString = FString::Printf(TEXT("%.0f"), curHp);
        HpText->SetText(FText::FromString(HpString));
    }

    UpdateGroggyBar(GroggyBar);
}


