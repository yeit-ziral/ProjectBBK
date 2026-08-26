// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BTTaskMeleeAutoAttack.h"
#include "AIController.h"
#include "../C_BaseMonster.h"

UC_BTTaskMeleeAutoAttack::UC_BTTaskMeleeAutoAttack()
{
	NodeName = TEXT("Monster Auto Attack");
}

EBTNodeResult::Type UC_BTTaskMeleeAutoAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* aiController = OwnerComp.GetAIOwner();
	if (!aiController)
		return EBTNodeResult::Failed;

	// 몬스터 타입별 Cast 없이 베이스로 받는다 — 실제 공격은 TryAutoAttack() override가 담당
	AC_BaseMonster* monster = Cast<AC_BaseMonster>(aiController->GetPawn());
	if (!monster)
		return EBTNodeResult::Failed;

	if (!monster->CanAutoAttack())
		return EBTNodeResult::Failed;

	if (monster->IsPlayingAttackAnimation())
		return EBTNodeResult::Failed;

	return monster->TryAutoAttack() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
