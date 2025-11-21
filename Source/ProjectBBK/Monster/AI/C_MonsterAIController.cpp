// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"


AC_MonsterAIController::AC_MonsterAIController()
{
    bAttachToPawn = true;
}

void AC_MonsterAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (BehaviorTreeAsset)
    {
        RunBehaviorTree(BehaviorTreeAsset);
    }
}
