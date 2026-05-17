// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_BossStorm.generated.h"

class USphereComponent;
class UParticleSystemComponent;
class UGameplayEffect;


UCLASS()
class PROJECTBBK_API AC_BossStorm : public AActor
{
	GENERATED_BODY()

public:
	AC_BossStorm();

	void InitOrbit(const FVector& InCenter, float InRadius, float InStartAngleDeg, float InSpeed);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* root;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* damageSphere;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* stormParticle;

	UPROPERTY(EditDefaultsOnly, Category = "Storm")
	TSubclassOf<UGameplayEffect> damageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Storm")
	float stormLifeTime = 5.f;

	UFUNCTION()
	void OnDamageSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		 int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool bOrbit = false;
	FVector orbitCenter = FVector::ZeroVector;
	float orbitRadius = 800.f;
	float orbitAngle = 0.f;
	float orbitSpeed = 60.f;
};
