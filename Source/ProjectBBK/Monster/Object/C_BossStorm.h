// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_BossStorm.generated.h"

class USphereComponent;
class UParticleSystemComponent;
class UAudioComponent;
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

	// 소환~소멸 동안 반복 재생되는 불타는 소리 — 보스 레이저(NS_FireTongueLick)의 루프 사운드와 같은 에셋.
	// 루프 큐라 액터가 Destroy되면 컴포넌트와 함께 자동으로 멈춘다.
	UPROPERTY(VisibleAnywhere)
	UAudioComponent* stormLoopAudio;

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
