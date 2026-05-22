// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Kismet/GameplayStatics.h"
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
            // 다음 틱에 재시도 — OnPossess 시점에 플레이어가 아직 준비 안 됐을 수 있음
            GetWorldTimerManager().SetTimer(initTargetTimer, this,
                &AC_MonsterAIController::TrySetInitialTarget, 0.1f, false);
        }
    }
}

void AC_MonsterAIController::TrySetInitialTarget()
{
    APawn* player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!player) return;

    if (UBlackboardComponent* BB = GetBlackboardComponent())
        BB->SetValueAsObject(TEXT("TargetActor"), player);

    SetFocus(player);
}
