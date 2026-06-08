// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "C_BossStormPatternGA.generated.h"

class UAnimMontage;
class UGameplayEffect;

UCLASS()
class PROJECTBBK_API UC_BossStormPatternGA : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UC_BossStormPatternGA();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void SpawnStorms();

	// --- 내부 캐시 ---
	UPROPERTY()
	TArray<TObjectPtr<AActor>> cachedDecalActors;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> cachedStormActors;

	UPROPERTY()
	TArray<FVector> cachedSpawnPoints;

	UPROPERTY()
	float cachedBaseAngle = 0.f;

	UPROPERTY()
	float cachedSpawnRadius = 800.f;

	// --- 에디터 설정 ---

	// 마법진 대기 시간 (AC_BossStorm::stormDelay와 같은 값으로 설정)
	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	float stormDelay = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	float patternRadius = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	int32 circleCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	float decalSize = 200.f;

	// 스폰 후 발사까지 대기 시간 (이 시간 동안 마법진 위에 폭풍이 정지)
	UPROPERTY(EditDefaultsOnly, Category = "StormPattern|Projectile")
	float stormLaunchDelay = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern|Projectile")
	float stormFlySpeed = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern|Projectile")
	float stormMaxTravelDistance = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	TSubclassOf<AActor> stormActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern|Damage")
	TSubclassOf<UGameplayEffect> damageEffectClass;

	// DataTable Attack 스탯에 곱할 배율
	UPROPERTY(EditDefaultsOnly, Category = "StormPattern|Damage")
	float damageMultiplier = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	UMaterialInterface* magicCircleDecal;

	// 마법진 대기 중 재생할 몽타주 (stormDelay에 맞춰 rate 자동 조정)
	UPROPERTY(EditDefaultsOnly, Category = "StormPattern|Animation")
	UAnimMontage* summoningMontage = nullptr;

private:
	UFUNCTION()
	void OnStormsLaunched();

	FTimerHandle spawnTimerHandle;
	FTimerHandle launchTimerHandle;
	float savedMaxWalkSpeed = 0.f;
};
