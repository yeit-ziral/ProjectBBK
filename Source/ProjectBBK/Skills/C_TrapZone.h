// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "NiagaraSystem.h"
#include "C_TrapZone.generated.h"

class UCapsuleComponent;
class UDecalComponent;
class UPointLightComponent;

UCLASS()
class PROJECTBBK_API AC_TrapZone : public AActor
{
	GENERATED_BODY()

public:
	AC_TrapZone();

	/**
	 * GA_RanagedUnique에서 스폰 직후 호출.
	 * LifeTimer는 이 시점에 시작 — BeginPlay 시점엔 수치 미주입.
	 */
	UFUNCTION(BlueprintCallable, Category = "TrapZone")
	void InitTrap(
		UAbilitySystemComponent* InASC,
		float InDamageMagnitude,
		float InLifeTime,
		float InTriggerRadius,
		float InTriggerHalfHeight,
		float InDamageRadius,
		float InDamageHalfHeight
	);

	UFUNCTION(BlueprintCallable, Category = "TrapZone")
	void DestroyTrap();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TrapZone")
	UCapsuleComponent* TriggerCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TrapZone")
	UDecalComponent* DecalComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TrapZone")
	UPointLightComponent* PointLightComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TrapZone")
	UMaterialInterface* DecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TrapZone")
	UNiagaraSystem* ImpactNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TrapZone")
	FVector NiagaraScale = FVector(1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TrapZone")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:
	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC;

	float DamageMagnitude    = 0.f;
	float LifeTime           = 0.f;
	float TriggerCapsuleRadius     = 0.f;
	float TriggerCapsuleHalfHeight = 0.f;
	float DamageCapsuleRadius      = 0.f;
	float DamageCapsuleHalfHeight  = 0.f;

	bool bTriggered = false;

	FTimerHandle LifeTimerHandle;

	UFUNCTION()
	void OnTriggerOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	void ApplyDamageToOverlappingMonsters();
	void OnLifeTimeExpired();
};
