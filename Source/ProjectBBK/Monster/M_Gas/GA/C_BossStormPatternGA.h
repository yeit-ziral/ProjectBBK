// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "C_BossStormPatternGA.generated.h"

/**
 *
 */
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

	UPROPERTY()
	TArray<TObjectPtr<AActor>> cachedDecalActors;

	UPROPERTY()
	TArray<FVector> cachedSpawnPoints;

	UPROPERTY()
	FVector cachedOrbitCenter = FVector::ZeroVector;

	UPROPERTY()
	float cachedOrbitRadius = 800.f;

	UPROPERTY()
	float cachedBaseAngle = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	float patternRadius = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	int32 circleCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	float decalSize = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	float stormDelay = 5.f;

	// 초당 회전 각도 (기본 60 = 6초에 1바퀴)
	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	float orbitSpeed = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	TSubclassOf<AActor> stormActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
	UMaterialInterface* magicCircleDecal;
};
