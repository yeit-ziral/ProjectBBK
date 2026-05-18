// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BTTaskBossPatternAttack.h"
#include "AIController.h"
#include "../C_BossMonster.h"

UC_BTTaskBossPatternAttack::UC_BTTaskBossPatternAttack()
{
	NodeName = TEXT("Boss Pattern Attack");
}

EBTNodeResult::Type UC_BTTaskBossPatternAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* aiController = OwnerComp.GetAIOwner();
	if (!aiController)
		return EBTNodeResult::Failed;

	AC_BossMonster* boss = Cast<AC_BossMonster>(aiController->GetPawn());
	if (!boss)
		return EBTNodeResult::Failed;

	// 쿨다운 안됐으면 Failed → Selector가 노말 공격 Task로 넘어감
	if (!boss->CanPatternAttack())
		return EBTNodeResult::Failed;

	boss->BossPatternAttack();
	return EBTNodeResult::Succeeded;
}
