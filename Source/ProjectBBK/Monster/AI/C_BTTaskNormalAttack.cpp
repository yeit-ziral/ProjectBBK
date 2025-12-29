// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BTTaskNormalAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "../C_BaseMonster.h"   
#include "../M_Gas/GA/C_MeleeMonsterNormalAttackGA.h"

UC_BTTaskNormalAttack::UC_BTTaskNormalAttack()
{
	NodeName = TEXT("Normal Attack (GA)");
}

EBTNodeResult::Type UC_BTTaskNormalAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{

	AAIController* aiController = OwnerComp.GetAIOwner();
	if (!aiController)
	{
		return EBTNodeResult::Failed;
	}

	aiController->StopMovement();


	APawn* controlledPawn = aiController->GetPawn();
	if (!controlledPawn)
	{
		return EBTNodeResult::Failed;
	}


	AC_BaseMonster* monster = Cast<AC_BaseMonster>(controlledPawn);
	if (!monster)
	{
		return EBTNodeResult::Failed;
	}

	UAbilitySystemComponent* abilitySystem = monster->GetMonsterASC();
	if (!abilitySystem)
	{
		return EBTNodeResult::Failed;
	}

	TSubclassOf<UGameplayAbility> abilityClassToUse = normalAttackAbilityClass;
	if (!abilityClassToUse)
	{
		UE_LOG(LogTemp, Warning, TEXT("NormalAttackTask: normalAttackAbilityClass is NULL"));
		return EBTNodeResult::Failed;
	}

	const bool activated = abilitySystem->TryActivateAbilityByClass(abilityClassToUse);
	return activated ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;

	//AActor* targetActor = nullptr;
	//if (bUseBlackboardTarget)
	//{
	//	if (UBlackboardComponent* bb = OwnerComp.GetBlackboardComponent())
	//	{
	//		UObject* targetObj = bb->GetValueAsObject(BlackboardKey.SelectedKeyName);
	//		targetActor = Cast<AActor>(targetObj);
	//	}
	//}

	//TSubclassOf<UGameplayAbility> abilityClassToUse = normalAttackAbilityClass;
	//if (!abilityClassToUse)
	//{
	//	abilityClassToUse = UC_MeleeMonsterNormalAttackGA::StaticClass(); 
	//}

	//const bool activated = abilitySystem->TryActivateAbilityByClass(abilityClassToUse);

	//return EBTNodeResult::Succeeded;

}
