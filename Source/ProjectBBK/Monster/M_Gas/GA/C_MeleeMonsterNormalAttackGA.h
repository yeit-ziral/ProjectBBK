// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "C_MeleeMonsterNormalAttackGA.generated.h"

class UAnimMontage;
class UGameplayEffect;
class USoundBase;

/**
 *
 */
UCLASS()
class PROJECTBBK_API UC_MeleeMonsterNormalAttackGA : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UC_MeleeMonsterNormalAttackGA();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
protected:

	// ���� ��Ÿ��
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	UAnimMontage* attackMontage;

	// ������ ����� GE (GE_Damage)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	TSubclassOf<UGameplayEffect> damageEffectClass;

	// AnimNotify���� ���� GameplayEvent �±� (��Ʈ Ÿ�̹�)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|HitEvent")
	FGameplayTag hitEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	FGameplayTag setByCallerDamageTag;

	// 판정 구체 반경 (공격 사거리는 DataTable AttackRange 사용)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Trace")
	float traceRadius = 60.0f;

	// DataTable Attack 스탯에 곱할 배율
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	float damageMultiplier = 1.0f;

	// 노말 공격(팔 휘두르기) 시작 시 재생 (MM_Monster_Arm_Swing_one_time)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Sound")
	USoundBase* attackSound = nullptr;

	void PlayAttackMontageAndBindEvents();

	// ��������Ʈ�� �Լ���
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

	// ���� ������ ����
	void ApplyDamageToTarget(AActor* TargetActor);

};
