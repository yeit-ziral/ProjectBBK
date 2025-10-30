// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_SkillBase.h"
#include "C_TestSkill.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_TestSkill : public UC_SkillBase
{
	GENERATED_BODY()
	
public:
	UC_TestSkill();

protected:
	virtual void ExecuteSkill_Implementation() override;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float dashDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float dashSpeed = 2000.0f;
};
