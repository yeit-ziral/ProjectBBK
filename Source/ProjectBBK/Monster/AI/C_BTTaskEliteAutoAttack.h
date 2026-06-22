// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "C_BTTaskEliteAutoAttack.generated.h"

/**
 * 스페셜 쿨타임 여부에 따라 스페셜/노말 공격을 자동 선택하는 엘리트 BT 태스크
 */
UCLASS()
class PROJECTBBK_API UC_BTTaskEliteAutoAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UC_BTTaskEliteAutoAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
