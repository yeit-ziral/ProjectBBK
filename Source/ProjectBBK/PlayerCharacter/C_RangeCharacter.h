// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectBBK/PlayerCharacter/C_BasePlayerCharactor.h"
#include "C_RangeCharacter.generated.h"

UCLASS()
class PROJECTBBK_API AC_RangeCharacter : public AC_BasePlayerCharactor
{
	GENERATED_BODY()

public:
	AC_RangeCharacter(const class FObjectInitializer &ObjectInitializer);

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

	/**  AnimBP에서 AimOffset 구동에 사용. 카메라 pitch(-90 ~ 90) */
	UPROPERTY(BlueprintReadOnly, Category = "RangeCharacter|Aim")
	float aimPitch = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "RangeCharacter|Aim")
	float aimYaw = 0.0f;

private:
	void UpdateAimOffset();
};
