// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "C_MeleeMonsterNormalAttackGA.generated.h"

class UAnimMontage;
class UGameplayEffect;

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

	// 공격 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	UAnimMontage* attackMontage;

	// 데미지 적용용 GE (GE_Damage)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	TSubclassOf<UGameplayEffect> damageEffectClass;

	// AnimNotify에서 쏴줄 GameplayEvent 태그 (히트 타이밍)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|HitEvent")
	FGameplayTag hitEventTag;

	//SetByCaller로 데미지를 넘기고 싶을 때 쓸 태그
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	FGameplayTag setByCallerDamageTag;

	// 기본 데미지
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	float baseDamage = 10.0f;

	// 몽타주 재생 및 이벤트 대기 세팅
	void PlayAttackMontageAndBindEvents();

	// 델리게이트용 함수들
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

	// 실제 데미지 적용
	void ApplyDamageToTarget(AActor* TargetActor);

};
