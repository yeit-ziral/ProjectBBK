// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BTTaskEliteAutoAttack.h"
#include "AIController.h"
#include "../C_EliteMonster.h"

UC_BTTaskEliteAutoAttack::UC_BTTaskEliteAutoAttack()
{
	NodeName = TEXT("Elite Auto Attack");
}

EBTNodeResult::Type UC_BTTaskEliteAutoAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* aiController = OwnerComp.GetAIOwner();
	if (!aiController)
		return EBTNodeResult::Failed;

	AC_EliteMonster* monster = Cast<AC_EliteMonster>(aiController->GetPawn());
	if (!monster)
		return EBTNodeResult::Failed;

	// 쿨다운 안됐으면 Failed → Selector가 fallback(Reposition 등)으로 넘어감
	if (!monster->CanAutoAttack())
		return EBTNodeResult::Failed;

	// 이전 공격 애니메이션이 재생 중이면 재진입 차단
	if (monster->IsPlayingAttackAnimation())
		return EBTNodeResult::Failed;

	return monster->EliteAutoAttack() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
