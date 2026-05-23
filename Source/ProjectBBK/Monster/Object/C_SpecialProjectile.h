// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_SpecialProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UParticleSystemComponent;
class UParticleSystem;
class UNiagaraSystem;
class UAbilitySystemComponent;
class UGameplayEffect;

UENUM(BlueprintType)
enum class ESpecialProjectilePhase : uint8
{
	Rising,
	Falling
};

UCLASS()
class PROJECTBBK_API AC_SpecialProjectile : public AActor
{
	GENERATED_BODY()

public:
	AC_SpecialProjectile();

	// 스폰 직후 호출 — 타겟 위치, 호 높이, 이동 속도 설정
	UFUNCTION(BlueprintCallable, Category = "SpecialProjectile")
	void InitSpecialProjectile(FVector InTargetLocation, float InArcHeight, float InMoveSpeed);

	// GAS 데미지 파라미터 설정 — InitSpecialProjectile 직후 호출
	void InitGASDamage(UAbilitySystemComponent* InInstigatorASC,
	                   TSubclassOf<UGameplayEffect> InDamageGEClass,
	                   float InDamage,
	                   float InAoERadius = 300.f);

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* collision;

	// 미사일 메쉬
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* missileMesh;

	// 트레일 파티클 (Cascade)
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UParticleSystemComponent* trailEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UParticleSystem* trailSystem;

	// 타겟 도달 시 폭발 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* explosionEffect;

	// 타겟에 도달했을 때 BP에서 데미지/이펙트 처리
	UFUNCTION(BlueprintImplementableEvent, Category = "SpecialProjectile")
	void OnReachedTarget();

private:
	// 최종 목표 위치
	FVector targetLocation;

	// 상승 목표 위치 (스폰~타겟 중점 상공)
	FVector apexLocation;

	// 이동 속도 (cm/s)
	float moveSpeed = 800.f;

	// 현재 페이즈
	ESpecialProjectilePhase currentPhase = ESpecialProjectilePhase::Rising;

	// 목표 도달 판정 거리
	static constexpr float ArrivalThreshold = 50.f;

	bool bInitialized = false;

	void MoveToward(FVector Destination, float DeltaTime);
	void ApplyAoEDamage();

	TWeakObjectPtr<UAbilitySystemComponent> instigatorASC;
	TSubclassOf<UGameplayEffect> damageGEClass;
	float damageValue = 0.f;
	float aoERadius   = 300.f;
};
