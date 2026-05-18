// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "C_BTTaskBossPatternAttack.generated.h"

UCLASS()
class PROJECTBBK_API UC_BTTaskBossPatternAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UC_BTTaskBossPatternAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
