// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectBBK/Monster/Object/C_RangedProjectile.h"
#include "GameplayEffectTypes.h"
#include "C_PlayerRangedProjectile.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * 플레이어 원거리 투사체.
 * AC_RangedProjectile(이동 담당)을 상속받아 피격 시 GAS로 데미지를 적용한다.
 * C_RangeAttackGA에서 Initialize()를 호출해 필요한 정보를 주입할 것.
 */
UCLASS()
class PROJECTBBK_API AC_PlayerRangedProjectile : public AC_RangedProjectile
{
	GENERATED_BODY()

public:
	AC_PlayerRangedProjectile();

	/** GA에서 스폰 직후 호출. 데미지 관련 정보를 주입한다. */
	void Initialize(UAbilitySystemComponent* InstigatorASC,
	                AActor*                  InstigatorActor,
	                TSubclassOf<UGameplayEffect> DamageEffectClass,
	                float DamageAmount);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent,
	                         AActor* OtherActor,
	                         UPrimitiveComponent* OtherComp,
	                         int32 OtherBodyIndex,
	                         bool bFromSweep,
	                         const FHitResult& SweepResult);

	TWeakObjectPtr<UAbilitySystemComponent> instigatorASC;
	TWeakObjectPtr<AActor>                  instigatorActor;
	TSubclassOf<UGameplayEffect>            damageEffectClass;
	float                                   damageAmount = 0.0f;

	/** 관통 방지: 첫 번째 히트 후 추가 데미지를 막는다. */
	bool bAlreadyHit = false;
};
