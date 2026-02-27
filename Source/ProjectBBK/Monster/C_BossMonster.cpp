// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossMonster.h"
#include "M_Gas/C_MonsterASC.h"
#include "UI/C_BossMonsterHPWidget.h"


AC_BossMonster::AC_BossMonster()
{
    rowName = "Boss";

}

void AC_BossMonster::BeginPlay()
{
    Super::BeginPlay();

    monsterTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Monster.Type.Boss"));
    HpWidgetComponent->SetWidgetClass(UC_BossMonsterHPWidget::StaticClass());

    // 테스트용: BaseMonster에서 이미 GiveAbility 된 GA 실행
    if (monsterASC && bossStormPatternGA && bossBeamPatternGA)
    {
        monsterASC->TryActivateAbilityByClass(bossStormPatternGA);
        //monsterASC->TryActivateAbilityByClass(bossBeamPatternGA);
    }
}

TSubclassOf<UUserWidget> AC_BossMonster::GetHpWidgetClass()
{
    return UC_BossMonsterHPWidget::StaticClass();
}
