// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterBTService.h"
#include "AIController.h"    
#include "Kismet/GameplayStatics.h"
#include "../../PlayerCharacter/C_BasePlayerCharactor.h"  
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

    AActor* ClosestPlayer = nullptr;
    float BestDist = FLT_MAX;

    for (AActor* Player : Players)
    {
        const float Dist = FVector::Dist(Player->GetActorLocation(), MyPawn->GetActorLocation());
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
    }
    else
    {
        BB->ClearValue(TargetActorKey.SelectedKeyName);
    }
}
