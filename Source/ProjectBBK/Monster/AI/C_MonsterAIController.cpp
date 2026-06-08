// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "../C_BaseMonster.h"


AC_MonsterAIController::AC_MonsterAIController()
{
    bAttachToPawn = true;
}

void AC_MonsterAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (AC_BaseMonster* Monster = Cast<AC_BaseMonster>(InPawn))
    {
        if (UBehaviorTree* BT = Monster->GetBehaviorTree())
        {
            RunBehaviorTree(BT);
            // BT 서비스(C_MonsterBTService)의 OnBecomeRelevant가 즉시 타겟 탐색을 처리하므로
            // 별도 초기 타겟 설정 불필요
        }
    }
}
