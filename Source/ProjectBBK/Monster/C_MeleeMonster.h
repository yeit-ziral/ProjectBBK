// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_BaseMonster.h"
#include "C_MeleeMonster.generated.h"

/**
 *
 */
UCLASS()
class PROJECTBBK_API AC_MeleeMonster : public AC_BaseMonster
{
	GENERATED_BODY()

public:
	AC_MeleeMonster();

	virtual bool CanAutoAttack() const override;
	virtual bool IsPlayingAttackAnimation() const override;

	// BT에서 호출 — 공격 실행 시 true, 쿨타임/조건 미충족 시 false
	UFUNCTION(BlueprintCallable, Category = "Attack")
	bool MeleeAutoAttack();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	bool MeleeNormalAttack();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	bool MeleeSpecialAttack();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* meleeNormalMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* meleeSpecialMontage = nullptr;

	// 슬램 모션에서 실제 타격 프레임까지의 딜레이 (에디터에서 조절)
	UPROPERTY(EditAnywhere, Category = "Attack")
	float slamHitDelay = 0.8f;

	FTimerHandle slamTimerHandle;
};
