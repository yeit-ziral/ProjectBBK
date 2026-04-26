// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "../../Data/GridLaserPatternData.h"
#include "C_BossGridLaserPatternGA.generated.h"

class AC_BossGhostClone;
class AC_GridLaser;
class UC_MonsterASC;
class AC_BasePlayerCharactor;

UCLASS()
class PROJECTBBK_API UC_BossGridLaserPatternGA : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UC_BossGridLaserPatternGA();

	void HandleMarkedGhostDestroyed(AC_BossGhostClone* DestroyedClone);

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	void ComputeGridPositions(const FVector& Center, TArray<FVector>& OutH, TArray<FVector>& OutV);
	void SpawnClones(const TArray<FVector>& HPositions, const TArray<FVector>& VPositions);
	void PickMarkedClones();

	UFUNCTION()
	void OnChargeFinished();

	UFUNCTION()
	void OnPatternFinished();

	UPROPERTY(EditDefaultsOnly, Category = "GridLaserPattern")
	FDataTableRowHandle gridPatternRow;

	UPROPERTY(EditDefaultsOnly, Category = "GridLaserPattern")
	TSubclassOf<AC_BossGhostClone> ghostCloneClass;

	UPROPERTY(EditDefaultsOnly, Category = "GridLaserPattern")
	TSubclassOf<AC_GridLaser> gridLaserClass;

	UPROPERTY(EditDefaultsOnly, Category = "GridLaserPattern")
	TSubclassOf<UGameplayEffect> damageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "GridLaserPattern")
	UAnimMontage* chargeMontage;

	UPROPERTY(EditDefaultsOnly, Category = "GridLaserPattern")
	UAnimMontage* fireMontage;

	UPROPERTY()
	TArray<TWeakObjectPtr<AC_BossGhostClone>> horizontalClones;

	UPROPERTY()
	TArray<TWeakObjectPtr<AC_BossGhostClone>> verticalClones;

	UPROPERTY()
	TWeakObjectPtr<AC_BossGhostClone> markedHorizontal;

	UPROPERTY()
	TWeakObjectPtr<AC_BossGhostClone> markedVertical;

	UPROPERTY()
	TArray<TWeakObjectPtr<AC_GridLaser>> spawnedLasers;

	UPROPERTY()
	TWeakObjectPtr<AC_BasePlayerCharactor> playerRef;

	FGridLaserPatternRow cachedRow;

	FTimerHandle chargeTimerHandle;
	FTimerHandle patternEndTimerHandle;

	UC_MonsterASC* cachedMonsterASC = nullptr;
	AActor* cachedBossActor = nullptr;
};
