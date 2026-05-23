// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_BossStorm.generated.h"

class USphereComponent;
class UParticleSystemComponent;
class UGameplayEffect;
class UAbilitySystemComponent;

UCLASS()
class PROJECTBBK_API AC_BossStorm : public AActor
{
	GENERATED_BODY()

public:
	AC_BossStorm();

	// GA에서 호출 — inLaunchDelay초 후 플레이어 방향으로 발사
	void InitProjectile(UAbilitySystemComponent* InInstigatorASC,
	                    TSubclassOf<UGameplayEffect> InDamageGEClass,
	                    float InDamageValue,
	                    float inLaunchDelay, float inFlySpeed, float inMaxTravelDistance);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* root;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* damageSphere;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* stormParticle;

	UFUNCTION()
	void OnDamageSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UFUNCTION()
	void LaunchTowardPlayer();

	float flySpeed          = 700.f;
	float maxTravelDistance = 2500.f;
	float distanceTraveled  = 0.f;

	FVector flyDirection = FVector::ForwardVector;
	bool bFlying = false;
	bool bHit    = false;

	FTimerHandle launchTimerHandle;

	TWeakObjectPtr<UAbilitySystemComponent> instigatorASC;
	TSubclassOf<UGameplayEffect> damageGEClass;
	float damageValue = 0.f;
};
