// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BTTaskReposition.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../C_BaseMonster.h"

UC_BTTaskReposition::UC_BTTaskReposition()
{
	NodeName    = TEXT("Reposition");
	bNotifyTick = true;
}

EBTNodeResult::Type UC_BTTaskReposition::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* aiController = OwnerComp.GetAIOwner();
	if (!aiController) return EBTNodeResult::Failed;

	AC_BaseMonster* monster = Cast<AC_BaseMonster>(aiController->GetPawn());
	if (!monster) return EBTNodeResult::Failed;
	if (!monster->IsRepositionEnabled()) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	AActor* target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!target) target = aiController->GetFocusActor();
	if (!target) target = UGameplayStatics::GetPlayerPawn(aiController, 0);
	if (!target) return EBTNodeResult::Failed;

	if (UCharacterMovementComponent* move = monster->GetCharacterMovement())
	{
		// GA가 이동을 막고 있을 때(MaxWalkSpeed=0)는 덮어쓰지 않음
		if (move->MaxWalkSpeed > 0.f)
			move->MaxWalkSpeed = monster->GetRepositionSpeed();
	}

	// 첫 번째 실행 시에만 초기화 (이후 세션은 이전 상태 그대로 이어짐)
	const float now = monster->GetWorld()->GetTimeSeconds();
	if (monster->repositionNextFlipTime < 0.f)
	{
		monster->repositionStrafeSign   = FMath::RandBool() ? 1 : -1;
		// 첫 전환이 세션 중간에 일어나도록 FlipInterval 절반 시간 후로 설정
		monster->repositionNextFlipTime = now + monster->GetRepositionFlipInterval() * 0.5f;
	}

	return EBTNodeResult::InProgress;
}

void UC_BTTaskReposition::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* aiController = OwnerComp.GetAIOwner();
	if (!aiController) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	AC_BaseMonster* monster = Cast<AC_BaseMonster>(aiController->GetPawn());
	if (!monster) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	AActor* target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!target) target = aiController->GetFocusActor();
	if (!target) target = UGameplayStatics::GetPlayerPawn(aiController, 0);
	if (!target) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	// 공격 가능해지면 Succeeded → Selector가 루프하며 공격 브랜치 재시도
	if (monster->CanAutoAttack())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 공격 애니메이션 재생 중에는 이동 입력 생략 (방향 전환 타이머는 계속 진행)
	bool bAttacking = monster->IsPlayingAttackAnimation();

	// 방향 전환 — 세션 간 누적된 타이머 기준으로 판단
	const float now = monster->GetWorld()->GetTimeSeconds();
	if (now >= monster->repositionNextFlipTime)
	{
		monster->repositionStrafeSign   = -monster->repositionStrafeSign;
		monster->repositionNextFlipTime = now + monster->GetRepositionFlipInterval();
	}

	if (bAttacking) return;

	// GA가 이동을 막고 있을 때(MaxWalkSpeed=0)는 이동 입력 생략
	if (UCharacterMovementComponent* mc = monster->GetCharacterMovement())
		if (mc->MaxWalkSpeed <= 0.f) return;

	FVector monsterLoc = monster->GetActorLocation();
	FVector targetLoc  = target->GetActorLocation();
	FVector flat       = monsterLoc - targetLoc;
	flat.Z             = 0.f;
	float dist         = flat.Size();

	FVector radial  = flat.GetSafeNormal();
	FVector tangent = FVector::CrossProduct(FVector::UpVector, radial) * (float)monster->repositionStrafeSign;

	const float desiredRange = monster->GetRepositionDesiredRange();
	const float minRange     = monster->GetRepositionMinRange();
	const float band         = monster->GetRepositionBand();
	const float strafeWeight = monster->GetRepositionStrafeWeight();

	FVector radialComp = FVector::ZeroVector;
	if (dist > desiredRange + band)
		radialComp = -radial;
	else if (minRange > 0.f && dist < minRange)
		radialComp = radial;

	FVector dir = radialComp * (1.f - strafeWeight) + tangent * strafeWeight;

	// 서클링 중 앞뒤 드리프트 — sin 파형으로 전후로 조금씩 이동
	dir += radial * FMath::Sin(now * 1.1f) * 0.4f;

	if (!dir.IsNearlyZero())
		monster->AddMovementInput(dir.GetSafeNormal(), 1.f);
}

void UC_BTTaskReposition::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	AAIController* aiController = OwnerComp.GetAIOwner();
	if (!aiController) return;

	AC_BaseMonster* monster = Cast<AC_BaseMonster>(aiController->GetPawn());
	if (!monster) return;

	if (UCharacterMovementComponent* move = monster->GetCharacterMovement())
		move->MaxWalkSpeed = monster->GetMonsterAttributeSet()->GetmoveSpeed();
}
