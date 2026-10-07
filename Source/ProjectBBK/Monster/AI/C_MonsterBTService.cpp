// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterBTService.h"
#include "AIController.h"    
#include "Kismet/GameplayStatics.h"
#include "../../PlayerCharacter/C_BasePlayerCharactor.h"
#include "../C_BaseMonster.h"
#include "BehaviorTree/BlackboardComponent.h"  

UC_MonsterBTService::UC_MonsterBTService()
{
    NodeName = TEXT("Monster BT Service");
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
	Interval = 0.1f;
	RandomDeviation = 0.0f;
}

void UC_MonsterBTService::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnBecomeRelevant(OwnerComp, NodeMemory);
    // BT 시작 즉시 타겟 탐색 — 첫 Tick을 기다리지 않음
    TickNode(OwnerComp, NodeMemory, 0.f);
}

void UC_MonsterBTService::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AICon = OwnerComp.GetAIOwner();
    if (!AICon) return;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return;

    APawn* MyPawn = AICon->GetPawn();
    if (!MyPawn) return;

    TArray<AActor*> Players;
    UGameplayStatics::GetAllActorsOfClass(AICon, AC_BasePlayerCharactor::StaticClass(), Players);

    // 공터 소속 몬스터는 플레이어가 공터 밖으로 나가면 타겟을 놓는다 (복귀는 C_BTTaskIdleWander)
    const AC_BaseMonster* Monster = Cast<AC_BaseMonster>(MyPawn);
    // 방금 맞았으면 한동안 공터 범위·인식 거리와 무관하게 쫓는다 (공터 밖 원거리 공격에 반응)
    const bool bAggroed = Monster && Monster->IsChaseLeashIgnored();
    const bool bLeashed = Monster && Monster->bHasChaseLeash && !bAggroed;
    const UObject* CurrentTarget = BB->GetValueAsObject(TargetActorKey.SelectedKeyName);

    AActor* ClosestPlayer = nullptr;
    float BestDist = FLT_MAX;

    for (AActor* Player : Players)
    {
        APawn* PlayerPawn = Cast<APawn>(Player);
        if (!PlayerPawn || !PlayerPawn->IsPlayerControlled()) continue;

        if (bLeashed)
        {
            // 이미 쫓던 대상은 여유를 더 줘서 경계선에서 추적/포기가 번갈아 반복되지 않게 함
            const float Limit = Monster->chaseLeashRadius + (Player == CurrentTarget ? LeashReleaseMargin : 0.f);
            if (FVector::Dist2D(Player->GetActorLocation(), Monster->chaseLeashCenter) > Limit) continue;
        }

        const float Dist = FVector::Dist(Player->GetActorLocation(), MyPawn->GetActorLocation());
        if (!bAggroed && Dist > DetectionRange) continue;
        if (Dist < BestDist)
        {
            BestDist = Dist;
            ClosestPlayer = Player;
        }
    }

    if (ClosestPlayer)
    {
        BB->SetValueAsObject(TargetActorKey.SelectedKeyName, ClosestPlayer);
        BB->SetValueAsFloat(DistanceToTargetKey.SelectedKeyName, BestDist);
        AICon->SetFocus(ClosestPlayer);
    }
    else
    {
        BB->ClearValue(TargetActorKey.SelectedKeyName);
        AICon->ClearFocus(EAIFocusPriority::Gameplay);
    }
}
