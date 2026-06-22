// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "C_EliteNormalAttackGA.generated.h"

class UAnimMontage;
class UGameplayEffect;
struct FGameplayEventData;

/**
 * 엘리트 노말 공격 GA (독립 구현).
 * 두 스윙 몽타주(attackMontage1 / attackMontage2)를 활성화마다 번갈아 재생하고,
 * AnimNotify가 보내는 hitEventTag 시점에 전방 구체판정으로 GE 데미지를 적용한다.
 * 각 스윙 몽타주는 Return2Idle까지 포함(에디터에서 구성)하므로 몽타주 종료 = 공격 종료.
 */
UCLASS()
class PROJECTBBK_API UC_EliteNormalAttackGA : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UC_EliteNormalAttackGA();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 첫 번째 스윙 몽타주 (Return2Idle 포함)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	UAnimMontage* attackMontage1 = nullptr;

	// 두 번째 스윙 몽타주 (Return2Idle 포함) — attackMontage1과 교대
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	UAnimMontage* attackMontage2 = nullptr;

	// 데미지에 사용할 GE (GE_BasicDamage)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	TSubclassOf<UGameplayEffect> damageEffectClass;

	// SetByCaller 데미지 태그 (Data.Damage)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	FGameplayTag setByCallerDamageTag;

	// DataTable Attack 스탯에 곱할 배율
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	float damageMultiplier = 1.0f;

	// AnimNotify가 보내는 hit 타이밍 GameplayEvent 태그 (Event.Monster.Melee.Hit)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|HitEvent")
	FGameplayTag hitEventTag;

	// 전방 구체판정 반경 (사거리는 DataTable AttackRange 사용)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Trace")
	float traceRadius = 80.0f;

private:
	// true면 다음 발동 시 attackMontage2 사용
	bool bUseSecondMontage = false;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

	void ApplyDamageToTarget(AActor* TargetActor);
};
