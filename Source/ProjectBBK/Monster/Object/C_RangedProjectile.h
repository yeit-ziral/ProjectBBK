// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_RangedProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UParticleSystemComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class PROJECTBBK_API AC_RangedProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AC_RangedProjectile();

	UFUNCTION()
	void InitVelocity(const FVector& Direction);

	void InitProjectile(UAbilitySystemComponent* InInstigatorASC,
	                    TSubclassOf<UGameplayEffect> InDamageGEClass,
	                    float InDamage,
	                    const FVector& Direction);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* collision;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UParticleSystemComponent* particle;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UProjectileMovementComponent* projectileMovement;

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                          bool bFromSweep, const FHitResult& SweepResult);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void ApplyDamageToTarget(AActor* Target);

	TWeakObjectPtr<UAbilitySystemComponent> instigatorASC;
	TSubclassOf<UGameplayEffect> damageGEClass;
	float damageValue = 10.f;
	bool bAlreadyHit = false;
};
