// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_BossProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UParticleSystemComponent;
class UProjectileMovementComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class PROJECTBBK_API AC_BossProjectile : public AActor
{
	GENERATED_BODY()

public:
	AC_BossProjectile();

	void InitProjectile(UAbilitySystemComponent* InInstigatorASC,
	                    TSubclassOf<UGameplayEffect> InDamageGEClass,
	                    float InDamage,
	                    const FVector& FireDirection);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* collision;

	// Blueprint에서 메시 설정
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* mesh;

	// 비행 중 파티클 (Blueprint에서 Particle System 할당)
	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* flightVFX;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* projectileMovement;

	// 투사체 속도 (Blueprint에서 덮어쓰기 가능)
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float speed = 1500.f;

	// 중력 비율 — 0이면 직선, 0.5 정도면 야구공 같은 포물선
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float gravityScale = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float lifeSpan = 6.f;

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                          bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	                 UPrimitiveComponent* OtherComp, FVector NormalImpulse,
	                 const FHitResult& Hit);

private:
	void ApplyDamageToTarget(AActor* Target);

	TWeakObjectPtr<UAbilitySystemComponent> instigatorASC;
	TSubclassOf<UGameplayEffect> damageGEClass;
	float damageValue = 10.f;
	bool bAlreadyHit = false;
};
