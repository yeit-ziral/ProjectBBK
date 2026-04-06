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

	// BT에서 호출 — 스페셜 쿨타임 여부에 따라 스페셜/노말 자동 선택
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void MeleeAutoAttack();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void MeleeNormalAttack();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void MeleeSpecialAttack();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* meleeNormalMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* meleeSpecialMontage = nullptr;
};
