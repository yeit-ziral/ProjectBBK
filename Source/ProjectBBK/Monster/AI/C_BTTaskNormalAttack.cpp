// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BTTaskNormalAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "../C_BaseMonster.h"   

UC_BTTaskNormalAttack::UC_BTTaskNormalAttack()
{
	NodeName = TEXT("Normal Attack (GA)");
}

EBTNodeResult::Type UC_BTTaskNormalAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Type();
}
