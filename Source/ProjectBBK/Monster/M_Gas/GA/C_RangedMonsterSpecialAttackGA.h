// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "C_RangedMonsterSpecialAttackGA.generated.h"

class UAnimMontage;
class UGameplayEffect;

UCLASS()
class PROJECTBBK_API UC_RangedMonsterSpecialAttackGA : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UC_RangedMonsterSpecialAttackGA();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	UAnimMontage* attackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	TSubclassOf<AActor> specialProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	FName backSocketName = TEXT("Back");

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	FVector backOffset = FVector(-100.f, 0.f, 50.f);

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	float specialArcHeight = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Projectile")
	float specialProjectileSpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	TSubclassOf<UGameplayEffect> damageEffectClass;

	// DataTable Attack 스탯에 곱할 배율
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	float damageMultiplier = 0.2f;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	void SpawnSpecialProjectile();
};
