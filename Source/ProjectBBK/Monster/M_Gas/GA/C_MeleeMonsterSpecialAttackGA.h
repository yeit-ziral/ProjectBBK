// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "C_MeleeMonsterSpecialAttackGA.generated.h"

class UAnimMontage;
class UGameplayEffect;
class USoundBase;

UCLASS()
class PROJECTBBK_API UC_MeleeMonsterSpecialAttackGA : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UC_MeleeMonsterSpecialAttackGA();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	UAnimMontage* attackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	TSubclassOf<UGameplayEffect> damageEffectClass;

	// AnimNotify에서 보내는 GameplayEvent 태그 (슬램 타이밍)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|HitEvent")
	FGameplayTag hitEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	FGameplayTag setByCallerDamageTag;

	// 슬램 AOE 반경
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Slam")
	float slamRadius = 250.0f;

	// 스페셜 공격 데미지 배율 (DataTable Attack 스탯 기준)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Slam")
	float slamDamageMultiplier = 2.0f;

	// 수평 넉백 강도
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Slam")
	float knockbackStrength = 900.0f;

	// 수직(위로 뜨는) 강도
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Slam")
	float verticalKnockbackStrength = 400.0f;

	// 땅을 내려찍는 순간 재생 (MM_Monster_Ground_Slam)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Sound")
	USoundBase* slamSound = nullptr;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnHitEventReceived(FGameplayEventData Payload);

	void ApplySlamDamageAndKnockback();
};
