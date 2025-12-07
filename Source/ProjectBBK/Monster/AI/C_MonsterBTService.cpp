// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterBTService.h"
#include "AIController.h"    
#include "Kismet/GameplayStatics.h"
#include "../../PlayerCharacter/C_BasePlayerCharactor.h"  
#include "BehaviorTree/BlackboardComponent.h"  

UC_MonsterBTService::UC_MonsterBTService()
{
    NodeName = TEXT("Monster BT Service");
	bNotifyTick = true;
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
