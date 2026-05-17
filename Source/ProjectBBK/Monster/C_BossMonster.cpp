// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BossMonster.h"
#include "M_Gas/C_MonsterASC.h"
#include "UI/C_BossMonsterHPWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"

AC_BossMonster::AC_BossMonster()
{
}

void AC_BossMonster::BeginPlay()
{
    Super::BeginPlay();

    bAutoAccumulateGroggy = false;

    monsterTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Monster.Type.Boss"));

    if (HpWidgetComponent)
    {
        HpWidgetComponent->SetVisibility(false);
        HpWidgetComponent->SetHiddenInGame(true);
    }

    InitializeBossHpWidget();

    if (monsterASC)
    {
        monsterASC->GetGameplayAttributeValueChangeDelegate(
            UC_MonsterAttributeSet::GetcurHPAttribute()
        ).AddUObject(this, &AC_BossMonster::OnBossHpChanged);

        monsterASC->GetGameplayAttributeValueChangeDelegate(
            UC_MonsterAttributeSet::GetcurGroggyAttribute()
        ).AddUObject(this, &AC_BossMonster::OnBossGroggyChanged);
    }

    if (monsterASC)
    {
        monsterASC->RegisterGameplayTagEvent(
            FGameplayTag::RequestGameplayTag(FName("State.Invincible")),
            EGameplayTagEventType::NewOrRemoved
        ).AddWeakLambda(this, [this](const FGameplayTag& Tag, int32 NewCount)
        {
            OnInvincibleTagChanged(Tag, NewCount);
        });
    }

    // GA 등록 — BT Task에서 활성화
    if (monsterASC)
    {
        if (bossNormalAttackGA)
            monsterASC->GiveAbility(FGameplayAbilitySpec(bossNormalAttackGA, 1, 0));
        if (bossStormPatternGA)
            monsterASC->GiveAbility(FGameplayAbilitySpec(bossStormPatternGA, 1, 0));
        if (bossBeamPatternGA)
            monsterASC->GiveAbility(FGameplayAbilitySpec(bossBeamPatternGA, 1, 0));
        if (bossGridLaserPatternGA)
            monsterASC->GiveAbility(FGameplayAbilitySpec(bossGridLaserPatternGA, 1, 0));
    }
}

// ─── 공격 쿨다운 체크 ─────────────────────────────────────────────────────────

bool AC_BossMonster::CanNormalAttack() const
{
    return GetWorld()->GetTimeSeconds() - lastNormalAttackTime >= normalAttackInterval;
}

bool AC_BossMonster::CanPatternAttack() const
{
    return GetWorld()->GetTimeSeconds() - lastPatternAttackTime >= patternAttackInterval;
}

// ─── BT Task 호출 진입점 ──────────────────────────────────────────────────────

void AC_BossMonster::BossNormalAttack()
{
    if (!monsterASC || !bossNormalAttackGA) return;
    if (!CanNormalAttack()) return;

    lastNormalAttackTime = GetWorld()->GetTimeSeconds();
    monsterASC->TryActivateAbilityByClass(bossNormalAttackGA);
}

void AC_BossMonster::BossPatternAttack()
{
    if (!monsterASC) return;
    if (!CanPatternAttack()) return;

    lastPatternAttackTime = GetWorld()->GetTimeSeconds();

    // Storm → Beam → Storm 교대
    if (bNextPatternIsStorm)
    {
        if (bossStormPatternGA)
        {
            bNextPatternIsStorm = false;
            monsterASC->TryActivateAbilityByClass(bossStormPatternGA);
        }
    }
    else
    {
        if (bossBeamPatternGA)
        {
            bNextPatternIsStorm = true;
            monsterASC->TryActivateAbilityByClass(bossBeamPatternGA);
        }
    }
}

// ─── HP 위젯 ─────────────────────────────────────────────────────────────────

void AC_BossMonster::InitializeBossHpWidget()
{
    if (!bossHpWidgetClass)
        return;

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
        return;

    bossHpWidget = CreateWidget<UC_BossMonsterHPWidget>(PlayerController, bossHpWidgetClass);
    if (!bossHpWidget)
        return;

    bossHpWidget->AddToViewport();
    bossHpWidget->SetMaxHp(GetmaxHP());
    bossHpWidget->SetCurrentHp(GetcurHP());
    bossHpWidget->SetMaxGroggy(GetmaxGroggy());
    bossHpWidget->SetCurrentGroggy(GetcurGroggy());
    bossHpWidget->SetMonsterLevel(50);
    bossHpWidget->SetMonsterName(FText::FromName(GetRowName()));
}

void AC_BossMonster::RemoveBossHpWidget()
{
    if (!bossHpWidget)
        return;

    bossHpWidget->RemoveFromParent();
    bossHpWidget = nullptr;
}

// ─── 이벤트 콜백 ─────────────────────────────────────────────────────────────

void AC_BossMonster::OnBossHpChanged(const FOnAttributeChangeData& ChangeData)
{
    if (bossHpWidget)
        bossHpWidget->SetCurrentHp(ChangeData.NewValue);

    if (!bPhase2Triggered && monsterASC && bossGridLaserPatternGA)
    {
        const float maxHp = GetmaxHP();
        const float ratio = maxHp > 0.f ? ChangeData.NewValue / maxHp : 1.f;

        if (maxHp > 0.f && ratio < phase2HpRatio && ratio > 0.1f)
        {
            bPhase2Triggered = true;
            monsterASC->TryActivateAbilityByClass(bossGridLaserPatternGA);
        }
    }
}

void AC_BossMonster::OnBossGroggyChanged(const FOnAttributeChangeData& ChangeData)
{
    if (!bossHpWidget) return;
    bossHpWidget->SetCurrentGroggy(ChangeData.NewValue);
}

void AC_BossMonster::OnInvincibleTagChanged(const FGameplayTag& Tag, int32 NewCount)
{
    if (!bossHpWidget) return;
    bossHpWidget->SetInvincible(NewCount > 0);
}
