// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BTTaskBossNormalAttack.h"
#include "AIController.h"
#include "../C_BossMonster.h"

UC_BTTaskBossNormalAttack::UC_BTTaskBossNormalAttack()
{
	NodeName = TEXT("Boss Normal Attack");
}

EBTNodeResult::Type UC_BTTaskBossNormalAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* aiController = OwnerComp.GetAIOwner();
	if (!aiController)
		return EBTNodeResult::Failed;

	AC_BossMonster* boss = Cast<AC_BossMonster>(aiController->GetPawn());
	if (!boss)
		return EBTNodeResult::Failed;

	// 쿨다운 안됐으면 Failed → Selector가 Wait Task로 넘어감
	if (!boss->CanNormalAttack())
		return EBTNodeResult::Failed;

	boss->BossNormalAttack();
	return EBTNodeResult::Succeeded;
}
