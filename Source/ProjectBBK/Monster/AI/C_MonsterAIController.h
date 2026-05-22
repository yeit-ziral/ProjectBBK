// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "C_MonsterAIController.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API AC_MonsterAIController : public AAIController
{
	GENERATED_BODY()
public:
    AC_MonsterAIController();

protected:
    virtual void OnPossess(APawn* InPawn) override;

private:
    void TrySetInitialTarget();
    FTimerHandle initTargetTimer;
};
