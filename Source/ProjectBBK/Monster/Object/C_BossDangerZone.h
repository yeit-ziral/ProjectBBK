// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_BossDangerZone.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class PROJECTBBK_API AC_BossDangerZone : public AActor
{
	GENERATED_BODY()

public:
	AC_BossDangerZone();

	// Blueprint GA에서 호출 — 안전지대 위치/반경, 데미지 GE 설정
	UFUNCTION(BlueprintCallable, Category = "DangerZone")
	void InitDangerZone(
		UAbilitySystemComponent* InBossASC,
		TSubclassOf<UGameplayEffect> InDamageGEClass,
		FVector InSafeZoneCenter,
		float InSafeZoneRadius = 300.f,
		float InHealthPercent  = 0.2f,
		float InTickRate       = 1.0f);

	// 패턴 종료 시 GA에서 호출해 타이머 정리
	UFUNCTION(BlueprintCallable, Category = "DangerZone")
	void StopDangerZone();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void TickDamage();

	TWeakObjectPtr<UAbilitySystemComponent> bossASC;
	TSubclassOf<UGameplayEffect> damageGEClass;

	FVector safeZoneCenter = FVector::ZeroVector;
	float safeZoneRadius   = 300.f;
	float healthPercent    = 0.2f;

	FTimerHandle damageTimerHandle;
};
