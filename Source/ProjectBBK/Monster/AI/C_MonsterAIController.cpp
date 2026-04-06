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

            if (UBlackboardComponent* BB = GetBlackboardComponent())
            {
                AActor* Player0 = UGameplayStatics::GetPlayerPawn(this, 0);
                BB->SetValueAsObject(TEXT("TargetActor"), Player0);
                SetFocus(Player0);
            }
        }
    }
}
