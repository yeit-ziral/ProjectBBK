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

    // Phase 2 GA는 gamePlayAbilities 배열과 별개이므로 여기서 명시적으로 등록
    if (monsterASC && bossGridLaserPatternGA)
        monsterASC->GiveAbility(FGameplayAbilitySpec(bossGridLaserPatternGA, 1, 0));

    // 테스트: BaseMonster에서 이미 GiveAbility 한 GA 확인
    if (monsterASC && bossStormPatternGA && bossBeamPatternGA)
    {
        monsterASC->TryActivateAbilityByClass(bossStormPatternGA);
        //monsterASC->TryActivateAbilityByClass(bossBeamPatternGA);
    }

    // TODO: 테스트용 — Phase2 발동 확인 후 삭제
    if (monsterAttributeSet)
    {
        const float startHP = monsterAttributeSet->GetmaxHP() * 0.55f;
        monsterAttributeSet->SetcurHP(startHP);
        if (bossHpWidget)
            bossHpWidget->SetCurrentHp(startHP);
    }
}

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

void AC_BossMonster::OnBossHpChanged(const FOnAttributeChangeData& ChangeData)
{
    if (bossHpWidget)
        bossHpWidget->SetCurrentHp(ChangeData.NewValue);

    if (!bPhase2Triggered && monsterASC && bossGridLaserPatternGA)
    {
        const float maxHp = GetmaxHP();
        const float ratio = maxHp > 0.f ? ChangeData.NewValue / maxHp : 1.f;

        UE_LOG(LogTemp, Warning, TEXT("[BossPhase2] HP=%.0f/%.0f (%.1f%%) threshold=%.0f%%"),
            ChangeData.NewValue, maxHp, ratio * 100.f, phase2HpRatio * 100.f);

        if (maxHp > 0.f && ratio < phase2HpRatio && ratio > 0.1f)
        {
            bPhase2Triggered = true;
            UE_LOG(LogTemp, Warning, TEXT("[BossPhase2] Triggering GridLaser pattern"));
            const bool bActivated = monsterASC->TryActivateAbilityByClass(bossGridLaserPatternGA);
            UE_LOG(LogTemp, Warning, TEXT("[BossPhase2] TryActivate result: %s"), bActivated ? TEXT("SUCCESS") : TEXT("FAILED"));
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

void AC_BossMonster::RemoveBossHpWidget()
{
    if (!bossHpWidget)
        return;

    bossHpWidget->RemoveFromParent();
    bossHpWidget = nullptr;
}

