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
   
    UPROPERTY(EditAnywhere, Category = "Detection")
    float DetectionRange = 1500.0f;

    // 공터(AC_BaseMonster::chaseLeashRadius) 밖으로 나간 플레이어를 이 거리(cm)만큼은 더 쫓은 뒤 포기
    UPROPERTY(EditAnywhere, Category = "Detection", meta = (ClampMin = "0.0"))
    float LeashReleaseMargin = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetActorKey;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector DistanceToTargetKey;

    virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	
};
