// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_BaseMonster.h"
#include "C_BearMonster.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API AC_BearMonster : public AC_BaseMonster
{
	GENERATED_BODY()

public:
	AC_BearMonster();
	
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void BearNormalAttack();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void BearSpecialAttack();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* bearNormalMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* bearSpecialMontage = nullptr;
};
