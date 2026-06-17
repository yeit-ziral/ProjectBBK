// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "C_BossBeamPatternGA.generated.h"

class AC_BossBeam;
class UAnimMontage;
class UGameplayEffect;

UCLASS()
class PROJECTBBK_API UC_BossBeamPatternGA : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UC_BossBeamPatternGA();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 빔 Actor 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	TSubclassOf<AC_BossBeam> beamClass;

	// 빔 개수 (기본 3)
	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	int32 beamCount = 3;

	// GA 지속 시간 — AC_BossBeam::lifeTime과 맞출 것
	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	float beamDuration = 10.f;

	// 빔 스폰 전 차징 모션
	UPROPERTY(EditDefaultsOnly, Category = "Beam|Animation")
	UAnimMontage* chargingMontage = nullptr;

	// 빔 발사 중 재생할 모션 (루프 권장)
	UPROPERTY(EditDefaultsOnly, Category = "Beam|Animation")
	UAnimMontage* firingMontage = nullptr;

	// 마법진 대기 시간 — AC_BossBeam::previewTime과 동일하게 설정
	// 차징 몽타주가 이 시간에 맞춰 속도 조정됨
	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	float chargingDuration = 5.f;

	// 빔 데미지 GE (GE_BasicDamage 설정 권장)
	UPROPERTY(EditDefaultsOnly, Category = "Beam|Damage")
	TSubclassOf<UGameplayEffect> damageEffectClass;

	// 빔 데미지 틱 간격 (초) — 짧을수록 DPS 증가
	UPROPERTY(EditDefaultsOnly, Category = "Beam|Damage")
	float beamDamageTickRate = 0.5f;

	// 빔 판정 최대 사거리 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Beam|Damage")
	float beamRange = 2000.f;

	// DataTable Attack 스탯에 곱할 배율
	UPROPERTY(EditDefaultsOnly, Category = "Beam|Damage")
	float damageMultiplier = 0.2f;

private:
	void OnBeamDurationExpired();
	void StartFiringPhase();

	float savedMaxWalkSpeed = 0.f;

	FTimerHandle chargingTimerHandle;
	FTimerHandle beamDurationTimerHandle;
	TArray<TWeakObjectPtr<AC_BossBeam>> spawnedBeams;
};
