// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_BaseMonster.h"
#include "UI/C_BossMonsterHPWidget.h"
#include "C_BossMonster.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API AC_BossMonster : public AC_BaseMonster
{
	GENERATED_BODY()
	

public:

    AC_BossMonster();

protected:

    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UC_BossMonsterHPWidget> bossHpWidgetClass;

    UPROPERTY()
    UC_BossMonsterHPWidget* bossHpWidget;

    void InitializeBossHpWidget();
    void RemoveBossHpWidget();

    // 테스트용 패턴 GA
    UPROPERTY(EditDefaultsOnly, Category = "Boss")
    TSubclassOf<UGameplayAbility> bossStormPatternGA;

    UPROPERTY(EditDefaultsOnly, Category = "Boss")
    TSubclassOf<UGameplayAbility> bossBeamPatternGA;
};
