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

    virtual bool CanAutoAttack() const override;

    // 보스 HP UI 숨김/표시 (컷신 트리거에서 호출 — 컷신 종료 후 표시).
    // 위젯이 아직 생성되지 않았어도 플래그가 InitializeBossHpWidget에 반영됨
    void SetHpWidgetSuppressed(bool bSuppressed);

protected:
    virtual void BeginPlay() override;
    virtual void ExecuteDeathSequence() override;

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


private:
    // true면 보스 HP UI를 숨김 — 컷신 동안 유지, 종료 시 false로 표시
    bool bSuppressHpWidget = false;

    bool bPhase2Triggered = false;
    bool bNextPatternIsStorm = true; // Storm → Beam → Storm 교대

    float lastNormalAttackTime  = -999.f;
    float lastPatternAttackTime = -999.f;
};
