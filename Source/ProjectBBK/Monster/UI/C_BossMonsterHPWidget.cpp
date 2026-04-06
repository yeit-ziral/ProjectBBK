// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossMonsterHPWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UC_BossMonsterHPWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (BossNameText)
		BossNameText->SetJustification(ETextJustify::Center);
	if (BossHpBar)
		originalHpBarColor = BossHpBar->GetFillColorAndOpacity();
	UpdateWidget();
}

void UC_BossMonsterHPWidget::SetInvincible(bool bInvincible)
{
	if (!BossHpBar) return;

	if (bInvincible)
		BossHpBar->SetFillColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f));
	else
		BossHpBar->SetFillColorAndOpacity(originalHpBarColor);
}

void UC_BossMonsterHPWidget::UpdateWidget()
{
    if (BossLevelText)
    {
        BossLevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv.%d"), monsterLevel)));
        BossLevelText->SetVisibility(bUseStatusText ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
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
