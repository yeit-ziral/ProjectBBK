// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "C_EliteMonsterSpecialAttackGA_Swamp.generated.h"

class AC_EliteSwampZone;
class UAnimMontage;

/**
 * 엘리트 스페셜 공격 — 검은 수렁 장판 소환.
 * 몬스터 발밑에 AC_EliteSwampZone을 스폰해 몬스터에 부착(따라다님)하고,
 * 일정 시간 동안 반경 안의 플레이어에게 주기 데미지를 가하게 한다.
 * 시전 몽타주(castMontage)는 선택 — 있으면 재생 후 종료, 없으면 즉시 종료.
 */
UCLASS()
class PROJECTBBK_API UC_EliteMonsterSpecialAttackGA_Swamp : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UC_EliteMonsterSpecialAttackGA_Swamp();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// BP에서 BP_EliteSwampZone 지정
	UPROPERTY(EditDefaultsOnly, Category = "SwampZone")
	TSubclassOf<AC_EliteSwampZone> swampZoneClass;

	// 선택: 시전 몽타주 (없으면 즉시 종료)
	UPROPERTY(EditDefaultsOnly, Category = "SwampZone")
	UAnimMontage* castMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "SwampZone")
	float zoneRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "SwampZone")
	float healthPercentPerTick = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "SwampZone")
	float tickRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SwampZone")
	float duration = 5.0f;

private:
	UFUNCTION()
	void OnMontageEnded();

	void SpawnZone();
};
