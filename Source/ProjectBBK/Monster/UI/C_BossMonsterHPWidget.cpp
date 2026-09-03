// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossMonsterHPWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

UC_BossMonsterHPWidget::UC_BossMonsterHPWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 보스는 채움 텍스처가 FullHPBossBar(일반 몬스터의 OnlyHP와 여백이 다름)라 구간을 따로 잡는다.
	hpFillStart = 0.125561f;
	hpFillEnd   = 0.868460f;
	// 그로기 바는 일반 몬스터와 동일한 GroogyBar 텍스처 — 베이스 기본값 그대로 사용.
}

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
        const float HpRatio = (maxHp > 0.0f) ? (curHp / maxHp) : 0.0f;
        BossHpBar->SetPercent(MapFillPercent(HpRatio, hpFillStart, hpFillEnd));
    }

    UpdateGroggyBar(BossGroggyBar);
}
