// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BTTaskIdleWander.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../C_BaseMonster.h"
#include "../M_Gas/C_MonsterASC.h"
#include "../M_Gas/C_MonsterAttributeSet.h"

namespace
{
	enum class EWanderPhase : uint8 { Wait, Move };

	struct FIdleWanderMemory
	{
		EWanderPhase phase = EWanderPhase::Wait;
		float phaseEndTime = 0.f;

		// 태스크가 바꾼 이동 설정 — OnTaskFinished에서 되돌림
		bool bSpeedOverridden         = false;
		bool bRotationOverridden      = false;
		bool bSavedOrientToMovement   = false;
		bool bSavedUseControllerYaw   = false;
	};

	// 그로기/사망 중에는 배회 정지
	bool IsWanderBlocked(const AC_BaseMonster* Monster)
	{
		const UC_MonsterASC* asc = Monster->GetMonsterASC();
		if (!IsValid(asc)) return false;

		static const FGameplayTag deadTag   = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
		static const FGameplayTag groggyTag = FGameplayTag::RequestGameplayTag(FName("State.Groggy"));
		return asc->HasMatchingGameplayTag(deadTag) || asc->HasMatchingGameplayTag(groggyTag);
	}
}

UC_BTTaskIdleWander::UC_BTTaskIdleWander()
{
	NodeName              = TEXT("Idle Wander");
	bNotifyTick           = true;
	bNotifyTaskFinished   = true;
}

uint16 UC_BTTaskIdleWander::GetInstanceMemorySize() const
{
	return sizeof(FIdleWanderMemory);
}

bool UC_BTTaskIdleWander::HasTarget(UBehaviorTreeComponent& OwnerComp) const
{
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	return BB && BB->GetValueAsObject(TargetActorKey.SelectedKeyName) != nullptr;
}

bool UC_BTTaskIdleWander::PickWanderPoint(const AC_BaseMonster* Monster, FVector& OutPoint) const
{
	const FVector home    = Monster->idleHomeLocation;
	const FVector current = Monster->GetActorLocation();

	for (int32 attempt = 0; attempt < 8; ++attempt)
	{
		// sqrt — 원 안에서 균일 분포 (그냥 RandRange면 중심에 몰림)
		const float   dist  = wanderRadius * FMath::Sqrt(FMath::FRand());
		const float   angle = FMath::FRandRange(0.f, 2.f * PI);
		const FVector candidate = home + FVector(FMath::Cos(angle) * dist, FMath::Sin(angle) * dist, 0.f);

		if (FVector::Dist2D(candidate, current) >= minStepDistance)
		{
			OutPoint = candidate;
			return true;
		}
	}
	return false;
}

bool UC_BTTaskIdleWander::StartMove(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FIdleWanderMemory* mem = CastInstanceNodeMemory<FIdleWanderMemory>(NodeMemory);
	AAIController* aiController = OwnerComp.GetAIOwner();
	AC_BaseMonster* monster = aiController ? Cast<AC_BaseMonster>(aiController->GetPawn()) : nullptr;
	if (!monster) return false;

	FVector dest;
	if (!PickWanderPoint(monster, dest)) return false;

	// 네비메시 경로 우선 (벽·낭떠러지 회피), 네비메시가 없는 맵이면 직선 이동으로 폴백
	EPathFollowingRequestResult::Type res =
		aiController->MoveToLocation(dest, acceptanceRadius, true, true, true, false);
	if (res == EPathFollowingRequestResult::Failed)
		res = aiController->MoveToLocation(dest, acceptanceRadius, true, false, false, false);

	if (res != EPathFollowingRequestResult::RequestSuccessful) return false;

	mem->phase        = EWanderPhase::Move;
	mem->phaseEndTime = monster->GetWorld()->GetTimeSeconds() + maxMoveTime;
	return true;
}

EBTNodeResult::Type UC_BTTaskIdleWander::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// OnTaskFinished는 즉시 실패해도 호출되므로 메모리부터 초기화
	FIdleWanderMemory* mem = new (NodeMemory) FIdleWanderMemory();

	AAIController* aiController = OwnerComp.GetAIOwner();
	if (!aiController) return EBTNodeResult::Failed;

	AC_BaseMonster* monster = Cast<AC_BaseMonster>(aiController->GetPawn());
	if (!monster) return EBTNodeResult::Failed;

	if (HasTarget(OwnerComp)) return EBTNodeResult::Failed;

	if (UCharacterMovementComponent* move = monster->GetCharacterMovement())
	{
		// GA가 이동을 막고 있을 때(MaxWalkSpeed=0)는 덮어쓰지 않음
		if (move->MaxWalkSpeed > 0.f && monster->GetMonsterAttributeSet())
		{
			move->MaxWalkSpeed     = monster->GetMonsterAttributeSet()->GetmoveSpeed() * wanderSpeedRatio;
			mem->bSpeedOverridden  = true;
		}

		// 원거리/보스는 전투용으로 컨트롤러 Yaw를 따라가는데(플레이어 정면 유지),
		// 배회 중엔 포커스가 없어 옆걸음/순간회전이 되므로 이동 방향으로 부드럽게 돌도록 임시 전환
		if (monster->bUseControllerRotationYaw)
		{
			mem->bRotationOverridden    = true;
			mem->bSavedOrientToMovement = move->bOrientRotationToMovement;
			mem->bSavedUseControllerYaw = monster->bUseControllerRotationYaw;

			monster->bUseControllerRotationYaw = false;
			move->bOrientRotationToMovement    = true;
		}
	}

	// 대기부터 시작 — 여러 마리가 동시에 출발하지 않도록 랜덤 시간
	mem->phase        = EWanderPhase::Wait;
	mem->phaseEndTime = monster->GetWorld()->GetTimeSeconds() + FMath::FRandRange(minWaitTime, FMath::Max(minWaitTime, maxWaitTime));

	return EBTNodeResult::InProgress;
}

void UC_BTTaskIdleWander::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FIdleWanderMemory* mem = CastInstanceNodeMemory<FIdleWanderMemory>(NodeMemory);

	AAIController* aiController = OwnerComp.GetAIOwner();
	AC_BaseMonster* monster = aiController ? Cast<AC_BaseMonster>(aiController->GetPawn()) : nullptr;
	if (!monster) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

	// 플레이어를 인식하면 즉시 빠져나가 Selector가 전투 브랜치를 선택하게 함
	if (HasTarget(OwnerComp))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const float now = monster->GetWorld()->GetTimeSeconds();

	if (IsWanderBlocked(monster))
	{
		if (mem->phase == EWanderPhase::Move)
			aiController->StopMovement();
		mem->phase        = EWanderPhase::Wait;
		mem->phaseEndTime = now + minWaitTime;
		return;
	}

	switch (mem->phase)
	{
	case EWanderPhase::Wait:
		if (now >= mem->phaseEndTime)
		{
			// 이동 요청 실패(갈 곳 없음 등)면 이번 사이클 종료 — 다음 사이클에서 다시 대기 후 재시도
			if (!StartMove(OwnerComp, NodeMemory))
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		break;

	case EWanderPhase::Move:
		if (aiController->GetMoveStatus() == EPathFollowingStatus::Idle || now >= mem->phaseEndTime)
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		break;
	}
}

EBTNodeResult::Type UC_BTTaskIdleWander::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Aborted;
}

void UC_BTTaskIdleWander::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	FIdleWanderMemory* mem = CastInstanceNodeMemory<FIdleWanderMemory>(NodeMemory);

	AAIController* aiController = OwnerComp.GetAIOwner();
	AC_BaseMonster* monster = aiController ? Cast<AC_BaseMonster>(aiController->GetPawn()) : nullptr;
	if (!monster) return;

	// 걷던 도중 타겟이 잡힌 경우 — 배회 이동을 끊어야 전투 브랜치의 MoveTo가 깔끔하게 시작됨
	if (mem->phase == EWanderPhase::Move && aiController->GetMoveStatus() != EPathFollowingStatus::Idle)
		aiController->StopMovement();

	UCharacterMovementComponent* move = monster->GetCharacterMovement();
	if (!move) return;

	// 배회 중 GA가 0으로 막은 경우는 건드리지 않음
	if (mem->bSpeedOverridden && move->MaxWalkSpeed > 0.f && monster->GetMonsterAttributeSet())
		move->MaxWalkSpeed = monster->GetMonsterAttributeSet()->GetmoveSpeed();

	if (mem->bRotationOverridden)
	{
		monster->bUseControllerRotationYaw = mem->bSavedUseControllerYaw;
		move->bOrientRotationToMovement    = mem->bSavedOrientToMovement;
	}

	mem->bSpeedOverridden    = false;
	mem->bRotationOverridden = false;
}
