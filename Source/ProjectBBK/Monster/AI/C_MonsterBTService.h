// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "C_MonsterBTService.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_MonsterBTService : public UBTService
{
	GENERATED_BODY()

public:
	UC_MonsterBTService();

protected:
   
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetActorKey;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector DistanceToTargetKey;

    virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	
};
