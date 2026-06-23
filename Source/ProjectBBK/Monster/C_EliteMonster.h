// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_BaseMonster.h"
#include "Abilities/GameplayAbility.h"
#include "C_EliteMonster.generated.h"

/**
 * 엘리트 몬스터 — 노말 공격 1개 + 스페셜 공격 2개.
 * 보스와 동일하게 쿨다운을 클래스가 직접 관리하며(AttackManager 미사용),
 * 스킬은 모두 GA(GiveAbility → TryActivateAbilityByClass)로 실행한다.
 * 두 스페셜은 SpecialCooldown을 공유하며 번갈아 발동된다.
 */
UCLASS()
class PROJECTBBK_API AC_EliteMonster : public AC_BaseMonster
{
	GENERATED_BODY()

public:
	AC_EliteMonster();

	// BT에서 호출 — 스페셜 쿨타임 여부에 따라 스페셜/노말 자동 선택
	UFUNCTION(BlueprintCallable, Category = "Attack")
	bool EliteAutoAttack();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	bool EliteNormalAttack();

	// special1 ↔ special2 번갈아 발동 (SpecialCooldown 공유)
	UFUNCTION(BlueprintCallable, Category = "Attack")
	bool EliteSpecialAttack();

	bool CanNormalAttack()  const;
	bool CanSpecialAttack() const;

	virtual bool CanAutoAttack() const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// [디버그] true면 BT 없이 일정 간격으로 자동 공격(EliteAutoAttack: 스페셜 우선) — 테스트용. 출시/BT 연결 전 false로 둘 것.
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebugAutoAttack = false;

	// [디버그] 자동 노말 공격 간격(초)
	UPROPERTY(EditAnywhere, Category = "Debug")
	float debugAttackInterval = 3.0f;

	// BP에서 BPC_EliteNormalAttackGA 할당
	UPROPERTY(EditDefaultsOnly, Category = "Attack|GA")
	TSubclassOf<UGameplayAbility> normalAttackGAClass;

	// BP에서 BPC_EliteSpecial1GA 할당
	UPROPERTY(EditDefaultsOnly, Category = "Attack|GA")
	TSubclassOf<UGameplayAbility> special1GAClass;

	// BP에서 BPC_EliteSpecial2GA 할당
	UPROPERTY(EditDefaultsOnly, Category = "Attack|GA")
	TSubclassOf<UGameplayAbility> special2GAClass;

private:
	float lastNormalAttackTime  = -999.f;
	float lastSpecialAttackTime = -999.f;
	bool  bNextSpecialIsSecond  = false;   // special1 ↔ special2 교대 플래그
	float debugAttackAccum      = 0.f;     // [디버그] 자동 공격 누적 시간
};
