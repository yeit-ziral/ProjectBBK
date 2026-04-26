// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "C_StoneSpearProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;

/**
 * GA_StoneSpear가 발사하는 돌창 Projectile.
 * 몬스터 충돌 시 DamageGE + SlowedGE 적용 후 소멸.
 * 지면/오브젝트 충돌 시 즉시 소멸.
 * InitialLifeSpan = 10초.
 */
UCLASS()
class PROJECTBBK_API AC_StoneSpearProjectile : public AActor
{
	GENERATED_BODY()

public:
	AC_StoneSpearProjectile();

	/**
	 * GA_StoneSpear에서 스폰 직후 호출.
	 * @param InInstigatorASC    플레이어 ASC
	 * @param InInstigatorActor  플레이어 Actor
	 * @param InDamageGEClass    데미지 GE (Set by Caller, Data.Damage)
	 * @param InSlowedGEClass    감속 GE (State.Slowed 태그 부여 + MoveSpeed 감소)
	 * @param InDamage           데미지 수치
	 * @param Direction          발사 방향 (월드 벡터)
	 */
	UFUNCTION(BlueprintCallable, Category = "StoneSpear")
	void InitProjectile(
		UAbilitySystemComponent* InInstigatorASC,
		AActor* InInstigatorActor,
		TSubclassOf<UGameplayEffect> InDamageGEClass,
		TSubclassOf<UGameplayEffect> InSlowedGEClass,
		float InDamage,
		const FVector& Direction
	);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* ProjectileEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* ImpactEffect;

private:
	TWeakObjectPtr<UAbilitySystemComponent> InstigatorASC;
	TWeakObjectPtr<AActor> InstigatorActor;

	TSubclassOf<UGameplayEffect> DamageGEClass;
	TSubclassOf<UGameplayEffect> SlowedGEClass;
	float DamageMagnitude = 0.f;

	bool bHasHit = false;

	UFUNCTION()
	void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	void ApplyEffectsToTarget(AActor* TargetActor);

	// 이동·충돌 비활성화 후 임팩트 이펙트 재생 시간만큼 대기 후 소멸
	void HandleImpact();
};
