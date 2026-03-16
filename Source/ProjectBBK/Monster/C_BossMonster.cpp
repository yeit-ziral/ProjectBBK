// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossMonster.h"
#include "M_Gas/C_MonsterASC.h"
#include "UI/C_BossMonsterHPWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"

AC_BossMonster::AC_BossMonster()
{
    rowName = "BossMonster";

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


    // 테스트용: BaseMonster에서 이미 GiveAbility 된 GA 실행
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
    bossHpWidget->SetMonsterName(FText::FromName(rowName));
}

void AC_BossMonster::RemoveBossHpWidget()
{
    if (!bossHpWidget)
        return;

    bossHpWidget->RemoveFromParent();
    bossHpWidget = nullptr;
}

