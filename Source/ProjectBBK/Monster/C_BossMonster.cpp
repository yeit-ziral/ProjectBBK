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

    // TODO: 테스트용 무적 토글 — 확인 후 삭제
    GetWorldTimerManager().SetTimer(invincibleTestTimerHandle, this, &AC_BossMonster::ToggleInvincible, 10.0f, true);

    // TODO: 테스트용 데미지 — 확인 후 삭제
    GetWorldTimerManager().SetTimer(testDamageTimerHandle, this, &AC_BossMonster::ApplyTestDamage, 4.0f, true);


    // 테스트: BaseMonster에서 이미 GiveAbility 한 GA 확인
    if (monsterASC && bossStormPatternGA && bossBeamPatternGA)
    {
        monsterASC->TryActivateAbilityByClass(bossStormPatternGA);
        //monsterASC->TryActivateAbilityByClass(bossBeamPatternGA);
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

void AC_BossMonster::ApplyTestDamage()
{
    if (!monsterASC || !testTrueDamageGE)
        return;

    FGameplayEffectContextHandle context = monsterASC->MakeEffectContext();
    FGameplayEffectSpecHandle spec = monsterASC->MakeOutgoingSpec(testTrueDamageGE, 1.0f, context);
    if (!spec.IsValid()) return;

    spec.Data->SetSetByCallerMagnitude(
        FGameplayTag::RequestGameplayTag(FName("Data.Damage")), 300.0f);

    monsterASC->ApplyGameplayEffectSpecToSelf(*spec.Data);
}

void AC_BossMonster::ToggleInvincible()
{
    if (!monsterASC) return;

    FGameplayTag invincibleTag = FGameplayTag::RequestGameplayTag(FName("State.Invincible"));

    if (monsterASC->HasMatchingGameplayTag(invincibleTag))
        monsterASC->RemoveLooseGameplayTag(invincibleTag);
    else
        monsterASC->AddLooseGameplayTag(invincibleTag);
}

void AC_BossMonster::OnBossHpChanged(const FOnAttributeChangeData& ChangeData)
{
    if (!bossHpWidget) return;
    bossHpWidget->SetCurrentHp(ChangeData.NewValue);
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

