// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BTTaskRangedAutoAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "../C_RangedMonster.h"

UC_BTTaskRangedAutoAttack::UC_BTTaskRangedAutoAttack()
{
	NodeName = TEXT("Ranged Auto Attack");
}

EBTNodeResult::Type UC_BTTaskRangedAutoAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* aiController = OwnerComp.GetAIOwner();
	if (!aiController)
		return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
		return EBTNodeResult::Failed;

	// 타겟 없으면 공격 안 함
	UObject* target = BB->GetValueAsObject(TargetActorKey.SelectedKeyName);
	if (!target)
		return EBTNodeResult::Failed;

	AC_RangedMonster* monster = Cast<AC_RangedMonster>(aiController->GetPawn());
	if (!monster)
		return EBTNodeResult::Failed;

	if (!monster->CanAutoAttack())
		return EBTNodeResult::Failed;

	// 이전 공격 애니메이션이 재생 중이면 재진입 차단
	if (monster->IsPlayingAttackAnimation())
		return EBTNodeResult::Failed;

	monster->RangedAutoAttack();
	return EBTNodeResult::Succeeded;
}
