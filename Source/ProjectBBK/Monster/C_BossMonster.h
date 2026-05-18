// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_BaseMonster.h"
#include "UI/C_BossMonsterHPWidget.h"
#include "C_BossMonster.generated.h"

UCLASS()
class PROJECTBBK_API AC_BossMonster : public AC_BaseMonster
{
	GENERATED_BODY()

public:
    AC_BossMonster();

    // BT Task에서 호출
    void BossNormalAttack();
    void BossPatternAttack();

    bool CanNormalAttack()  const;
    bool CanPatternAttack() const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UC_BossMonsterHPWidget> bossHpWidgetClass;

    UPROPERTY()
    UC_BossMonsterHPWidget* bossHpWidget;

    void InitializeBossHpWidget();
    void RemoveBossHpWidget();

    void OnInvincibleTagChanged(const FGameplayTag& Tag, int32 NewCount);
    void OnBossHpChanged(const FOnAttributeChangeData& ChangeData);
    void OnBossGroggyChanged(const FOnAttributeChangeData& ChangeData);

    UPROPERTY(EditDefaultsOnly, Category = "Boss")
    TSubclassOf<UGameplayAbility> bossStormPatternGA;

    UPROPERTY(EditDefaultsOnly, Category = "Boss")
    TSubclassOf<UGameplayAbility> bossBeamPatternGA;

    UPROPERTY(EditDefaultsOnly, Category = "Boss")
    TSubclassOf<UGameplayAbility> bossNormalAttackGA;

    UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase")
    TSubclassOf<UGameplayAbility> bossGridLaserPatternGA;

    UPROPERTY(EditDefaultsOnly, Category = "Boss|Phase")
    float phase2HpRatio = 0.5f;

    // 노말 공격 간격 (초)
    UPROPERTY(EditDefaultsOnly, Category = "Boss|Cooldown")
    float normalAttackInterval = 3.f;

    // 패턴 공격 간격 (초)
    UPROPERTY(EditDefaultsOnly, Category = "Boss|Cooldown")
    float patternAttackInterval = 10.f;

private:
    bool bPhase2Triggered = false;
    bool bNextPatternIsStorm = true; // Storm → Beam → Storm 교대

    float lastNormalAttackTime  = -999.f;
    float lastPatternAttackTime = -999.f;
};
