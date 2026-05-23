// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "../../Object/C_RangedProjectile.h"

#include "C_RangedMonsterNormalAttackGA.generated.h"

class UGameplayEffect;

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_RangedMonsterNormalAttackGA : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UC_RangedMonsterNormalAttackGA();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* AttackMontage;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
       const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnFireProjectileEvent(FGameplayEventData Payload);

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<class AC_RangedProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<UGameplayEffect> damageEffectClass;
};
