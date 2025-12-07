// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "C_MonsterAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_MonsterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
    UC_MonsterAnimInstance();

	virtual void NativeBeginPlay() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:

    UFUNCTION(BlueprintCallable, Category = "Movement")
    float GetSpeed() const { return Speed; }

    UFUNCTION(BlueprintCallable, Category = "Movement")
    float GetDir() const { return Dir; }
protected:


    UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float Speed;


    UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float Dir;

    UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    APawn* OwnerPawn;

	
};
