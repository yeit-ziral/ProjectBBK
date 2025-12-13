// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "C_BTTaskNormalAttack.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UC_MeleeMonsterNormalAttackGA;
/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_BTTaskNormalAttack : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
		UC_BTTaskNormalAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
protected:
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool bUseBlackboardTarget = true;

	UPROPERTY(EditAnywhere, Category = "Attack")
	TSubclassOf<UGameplayAbility> normalAttackAbilityClass;
	
};
