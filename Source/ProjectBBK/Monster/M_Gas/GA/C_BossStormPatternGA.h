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

    void GenerateStormTriangle(const FVector& Center, float Radius, TArray<FVector>& OutPoints);

    UFUNCTION()
    void SpawnStorms();
	
    UPROPERTY()
    TArray<FVector> cachedSpawnPoints;

    UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
    float patternRadius = 1200.f;

    UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
    float minDistanceBetweenCircles = 300.f;

    UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
    int32 circleCount = 3;

    UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
    float decalSize = 200.f;

    UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
    float stormDelay = 5.f;

    UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
    TSubclassOf<AActor> stormActorClass;

    UPROPERTY(EditDefaultsOnly, Category = "StormPattern")
    UMaterialInterface* magicCircleDecal;
};
