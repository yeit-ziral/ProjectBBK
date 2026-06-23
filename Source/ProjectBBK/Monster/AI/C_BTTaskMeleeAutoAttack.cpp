// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BTTaskMeleeAutoAttack.h"
#include "AIController.h"
#include "../C_MeleeMonster.h"
#include "../Manager/C_AttackManagerComponent.h"

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

	if (monster->IsPlayingAttackAnimation())
		return EBTNodeResult::Failed;

	return monster->MeleeAutoAttack() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
