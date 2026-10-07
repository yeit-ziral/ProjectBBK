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
	enum class EWanderPhase : uint8 { Wait, Turn, Move };

	// 이 각도(도) 안으로 들어오면 제자리 회전을 끝내고 걷기 시작 — 남은 각도는 걸으면서 맞춤
	const float TURN_DONE_ANGLE = 10.f;
	// 제자리 회전이 끝나지 않을 때(밀림 등) 강제로 걷기로 넘어가는 시간 (초)
	const float TURN_TIMEOUT    = 3.f;
	// 화면 밖일 때 다시 확인하는 주기 (초)
	const float OFFSCREEN_RECHECK_TIME = 1.f;

	struct FIdleWanderMemory
	{
		EWanderPhase phase = EWanderPhase::Wait;
		float phaseEndTime = 0.f;
		FVector destination = FVector::ZeroVector;

		// 태스크가 바꾼 이동 설정 — OnTaskFinished에서 되돌림
		bool bSpeedOverridden         = false;
		bool bRotationOverridden      = false;
		bool bSavedOrientToMovement   = false;
		bool bSavedUseControllerYaw   = false;

		bool     bSmoothingOverridden          = false;
		float    savedMaxAcceleration          = 0.f;
		FRotator savedRotationRate             = FRotator::ZeroRotator;
		bool     bSavedUseAccelerationForPaths = false;
		bool     bSavedUseFixedBrakingDistance = false;
		float    savedFixedBrakingDistance     = 0.f;
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

	const FVector forward = Monster->GetActorForwardVector().GetSafeNormal2D();
	const float   minDot  = FMath::Cos(FMath::DegreesToRadians(preferredTurnAngle));

	// 선호 각도 안의 지점이 안 나오면(홈 반경 가장자리에서 바깥을 보고 있는 경우 등) 가장 덜 도는 지점을 사용
	bool  bHasFallback = false;
	float bestDot      = -2.f;

	for (int32 attempt = 0; attempt < 12; ++attempt)
	{
		// sqrt — 원 안에서 균일 분포 (그냥 RandRange면 중심에 몰림)
		const float   dist  = wanderRadius * FMath::Sqrt(FMath::FRand());
		const float   angle = FMath::FRandRange(0.f, 2.f * PI);
		const FVector candidate = home + FVector(FMath::Cos(angle) * dist, FMath::Sin(angle) * dist, 0.f);

		if (FVector::Dist2D(candidate, current) < minStepDistance) continue;

		const float dot = FVector::DotProduct(forward, (candidate - current).GetSafeNormal2D());
		if (dot >= minDot)
		{
			OutPoint = candidate;
			return true;
		}

		if (dot > bestDot)
		{
			bestDot      = dot;
			OutPoint     = candidate;
			bHasFallback = true;
		}
	}
	return bHasFallback;
}

bool UC_BTTaskIdleWander::StartTurn(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FIdleWanderMemory* mem = CastInstanceNodeMemory<FIdleWanderMemory>(NodeMemory);
	AAIController* aiController = OwnerComp.GetAIOwner();
	AC_BaseMonster* monster = aiController ? Cast<AC_BaseMonster>(aiController->GetPawn()) : nullptr;
	if (!monster) return false;

	if (!PickWanderPoint(monster, mem->destination)) return false;

	mem->phase        = EWanderPhase::Turn;
	mem->phaseEndTime = monster->GetWorld()->GetTimeSeconds() + TURN_TIMEOUT;
	return true;
}

bool UC_BTTaskIdleWander::StartMove(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float MoveTimeout)
{
	FIdleWanderMemory* mem = CastInstanceNodeMemory<FIdleWanderMemory>(NodeMemory);
	AAIController* aiController = OwnerComp.GetAIOwner();
	AC_BaseMonster* monster = aiController ? Cast<AC_BaseMonster>(aiController->GetPawn()) : nullptr;
	if (!monster) return false;

	const FVector dest = mem->destination;

	// 네비메시 경로 우선 (벽·낭떠러지 회피), 네비메시가 없는 맵이면 직선 이동으로 폴백
	EPathFollowingRequestResult::Type res =
		aiController->MoveToLocation(dest, acceptanceRadius, true, true, true, false);
	if (res == EPathFollowingRequestResult::Failed)
		res = aiController->MoveToLocation(dest, acceptanceRadius, true, false, false, false);

	if (res != EPathFollowingRequestResult::RequestSuccessful) return false;

	mem->phase        = EWanderPhase::Move;
	mem->phaseEndTime = monster->GetWorld()->GetTimeSeconds() + MoveTimeout;
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

	// 추적하다 타겟을 놓쳐 배회 반경 밖에 있으면 이번 사이클은 배회가 아니라 스폰 위치 복귀
	const bool bReturnHome =
		FVector::Dist2D(monster->GetActorLocation(), monster->idleHomeLocation) > wanderRadius + returnHomeMargin;

	if (UCharacterMovementComponent* move = monster->GetCharacterMovement())
	{
		// GA가 이동을 막고 있을 때(MaxWalkSpeed=0)는 덮어쓰지 않음. 복귀는 전투 이동 속도 그대로
		if (!bReturnHome && move->MaxWalkSpeed > 0.f && monster->GetMonsterAttributeSet())
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

		// 출발·정지·회전을 부드럽게 — 전투용 값(가속 2048, 회전 720도/초, 속도 직접 대입)은 배회엔 너무 급함.
		// 경로 추종을 가속 기반으로 바꾸면 도착 시 속도를 0으로 강제하지 않고 브레이킹 거리 안에서 서서히 줄어듦
		FNavMovementProperties* navProps = bReturnHome ? nullptr : move->GetNavMovementProperties();
		if (navProps)
		{
			mem->bSmoothingOverridden          = true;
			mem->savedMaxAcceleration          = move->MaxAcceleration;
			mem->savedRotationRate             = move->RotationRate;
			mem->bSavedUseAccelerationForPaths = navProps->bUseAccelerationForPaths;
			mem->bSavedUseFixedBrakingDistance = navProps->bUseFixedBrakingDistanceForPaths;
			mem->savedFixedBrakingDistance     = navProps->FixedPathBrakingDistance;

			move->MaxAcceleration = wanderAcceleration;
			move->RotationRate    = FRotator(0.f, wanderTurnRate, 0.f);
			navProps->bUseAccelerationForPaths         = true;
			navProps->bUseFixedBrakingDistanceForPaths = true;
			navProps->FixedPathBrakingDistance         = wanderBrakingDistance;
		}
	}

	// 복귀는 대기·제자리 회전 없이 바로 출발. 이동 요청이 실패하면 아래 일반 대기로 넘어가 다음 사이클에 재시도
	if (bReturnHome && !IsWanderBlocked(monster))
	{
		mem->destination = monster->idleHomeLocation;
		if (StartMove(OwnerComp, NodeMemory, returnMaxMoveTime))
			return EBTNodeResult::InProgress;
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
			// 화면에 안 보이면 걸을 필요가 없음 — 서 있기만 하고 잠시 뒤 다시 확인
			if (bWanderOnlyWhenRendered && !monster->WasRecentlyRendered(0.5f))
			{
				mem->phaseEndTime = now + OFFSCREEN_RECHECK_TIME;
				break;
			}

			// 갈 곳이 없으면 이번 사이클 종료 — 다음 사이클에서 다시 대기 후 재시도
			if (!StartTurn(OwnerComp, NodeMemory))
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		break;

	case EWanderPhase::Turn:
	{
		// 목표 방향을 먼저 바라본 뒤 출발 — 걸으면서 돌면 그동안 옆/뒤로 미끄러지며 Dir 블렌드스페이스가 튐
		const FVector  toDest  = (mem->destination - monster->GetActorLocation()).GetSafeNormal2D();
		const FRotator current = monster->GetActorRotation();
		const FRotator target(current.Pitch, toDest.Rotation().Yaw, current.Roll);
		const float    remain  = FMath::Abs(FMath::FindDeltaAngleDegrees(current.Yaw, target.Yaw));

		if (toDest.IsNearlyZero() || remain <= TURN_DONE_ANGLE || now >= mem->phaseEndTime)
		{
			// 이동 요청 실패면 이번 사이클 종료
			if (!StartMove(OwnerComp, NodeMemory, maxMoveTime))
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		else
		{
			monster->SetActorRotation(FMath::RInterpConstantTo(current, target, DeltaSeconds, wanderTurnRate));
		}
		break;
	}

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

	if (mem->bSmoothingOverridden)
	{
		move->MaxAcceleration = mem->savedMaxAcceleration;
		move->RotationRate    = mem->savedRotationRate;

		if (FNavMovementProperties* navProps = move->GetNavMovementProperties())
		{
			navProps->bUseAccelerationForPaths         = mem->bSavedUseAccelerationForPaths;
			navProps->bUseFixedBrakingDistanceForPaths = mem->bSavedUseFixedBrakingDistance;
			navProps->FixedPathBrakingDistance         = mem->savedFixedBrakingDistance;
		}
	}

	mem->bSpeedOverridden     = false;
	mem->bRotationOverridden  = false;
	mem->bSmoothingOverridden = false;
}
