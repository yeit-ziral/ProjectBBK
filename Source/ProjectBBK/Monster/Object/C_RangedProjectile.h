// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_RangedProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UParticleSystemComponent;

UCLASS()
class PROJECTBBK_API AC_RangedProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AC_RangedProjectile();

	UFUNCTION()
	void InitVelocity(const FVector& Direction);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* collision;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UParticleSystemComponent* particle;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UProjectileMovementComponent* projectileMovement;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
