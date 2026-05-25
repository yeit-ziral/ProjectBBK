// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BTTaskMeleeAutoAttack.h"
#include "AIController.h"
#include "../C_MeleeMonster.h"

UC_BTTaskMeleeAutoAttack::UC_BTTaskMeleeAutoAttack()
{
	NodeName = TEXT("Melee Auto Attack");
}

EBTNodeResult::Type UC_BTTaskMeleeAutoAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* aiController = OwnerComp.GetAIOwner();
	if (!aiController)
		return EBTNodeResult::Failed;

	AC_MeleeMonster* monster = Cast<AC_MeleeMonster>(aiController->GetPawn());
	if (!monster)
		return EBTNodeResult::Failed;

	if (!monster->CanAutoAttack())
		return EBTNodeResult::Failed;

	// 이전 공격 애니메이션이 재생 중이면 재진입 차단
	if (monster->IsPlayingAttackAnimation())
		return EBTNodeResult::Failed;

	// 공격이 실제로 실행된 경우에만 Succeeded — 쿨타임/ID 불일치로 미실행 시 Failed
	return monster->MeleeAutoAttack() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
