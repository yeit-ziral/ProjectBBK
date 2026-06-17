// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "C_BossNormalAttackGA.generated.h"

class AC_BossProjectile;

UCLASS()
class PROJECTBBK_API UC_BossNormalAttackGA : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UC_BossNormalAttackGA();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnThrowEvent(FGameplayEventData Payload);

	// 던지기 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	UAnimMontage* attackMontage;

	// BPC_BossProjectile 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<AC_BossProjectile> projectileClass;

	// GE_BasicDamage 또는 전용 데미지 GE
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> damageGEClass;

	// DataTable Attack 스탯에 곱할 배율
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float damageMultiplier = 0.1f;

	// AnimNotify(C_SpawnProjectile_AnimNotify)의 FireEventTag와 동일하게 설정
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FGameplayTag throwEventTag;

	// 투사체가 생성될 소켓 이름 (보스 스켈레톤에 맞게 설정)
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FName throwSocketName = TEXT("hand_r");
};
