// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "C_ExpOrb.generated.h"

class USphereComponent;
class UNiagaraComponent;

UCLASS()
class PROJECTBBK_API AC_ExpOrb : public AActor
{
	GENERATED_BODY()

public:
	AC_ExpOrb();

	UFUNCTION(BlueprintCallable)
	void InitOrb(float InExpAmount);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere)
	UNiagaraComponent* NiagaraEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ExpAmount = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> GE_GainExperience;

private:
	bool bConsumed = false;

	UFUNCTION()
	void OnOrbOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
};
