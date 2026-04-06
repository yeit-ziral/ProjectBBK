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

    void OnInvincibleTagChanged(const FGameplayTag& Tag, int32 NewCount);

    void OnBossHpChanged(const FOnAttributeChangeData& ChangeData);
    void OnBossGroggyChanged(const FOnAttributeChangeData& ChangeData);

    // TODO: 테스트용 무적 토글 — 확인 후 삭제
    void ToggleInvincible();
    FTimerHandle invincibleTestTimerHandle;


    // TODO: 테스트용 데미지 — 확인 후 삭제
    void ApplyTestDamage();
    FTimerHandle testDamageTimerHandle;

    UPROPERTY(EditDefaultsOnly, Category = "Boss|Test")
    TSubclassOf<UGameplayEffect> testDamageGE;

    UPROPERTY(EditDefaultsOnly, Category = "Boss|Test")
    TSubclassOf<UGameplayEffect> testTrueDamageGE;
    // 테스트용 보스 GA
    UPROPERTY(EditDefaultsOnly, Category = "Boss")
    TSubclassOf<UGameplayAbility> bossStormPatternGA;

    UPROPERTY(EditDefaultsOnly, Category = "Boss")
    TSubclassOf<UGameplayAbility> bossBeamPatternGA;
};
