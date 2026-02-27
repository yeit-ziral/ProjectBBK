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
	// Sets default values for this actor's properties
	AC_BossStorm();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
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
};
