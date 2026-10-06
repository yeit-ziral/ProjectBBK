// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "C_BTTaskIdleWander.generated.h"

// 비전투(타겟 없음) 배회 — 스폰 위치(AC_BaseMonster::idleHomeLocation) 주변 반경 안에서
// "잠깐 대기 → 근처 지점으로 천천히 걸어감"을 한 사이클로 수행하고 Succeeded를 반환한다.
// 루트 Selector의 마지막(최저 우선순위) 자식으로 배치 — 타겟이 생기면 즉시 Succeeded로 빠져나감.
UCLASS()
class PROJECTBBK_API UC_BTTaskIdleWander : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UC_BTTaskIdleWander();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	// 타겟이 잡히면 배회 중단 (C_MonsterBTService가 채우는 키)
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	// 스폰 위치 기준 배회 반경 (cm)
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "50.0"))
	float wanderRadius = 250.f;

	// 한 번에 이동할 최소 거리 — 너무 짧으면 제자리 발걸음처럼 보임
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0"))
	float minStepDistance = 100.f;

	// 이동 전 대기 시간 범위 (초)
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0"))
	float minWaitTime = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0"))
	float maxWaitTime = 4.0f;

	// 배회 속도 = DT moveSpeed × 이 비율 (스탯 하드코딩 금지 규칙 — 비율만 노드에서 조정)
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float wanderSpeedRatio = 0.3f;

	// 경로가 막혀 도착 못 할 때 사이클을 끊는 시간 (초)
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.5"))
	float maxMoveTime = 5.f;

	// 목표 지점 도착 판정 반경 (cm)
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "5.0"))
	float acceptanceRadius = 30.f;

	// 걷기 전에 제자리에서 목표 방향으로 도는 속도 (도/초) — 걷는 중 회전에도 같은 값 사용.
	// 전투용 RotationRate(720)를 그대로 쓰면 출발 순간 몸이 홱 돌아가고, 도는 동안 옆/뒷걸음 애니메이션이 섞임
	UPROPERTY(EditAnywhere, Category = "Wander|Smoothing", meta = (ClampMin = "30.0"))
	float wanderTurnRate = 200.f;

	// 현재 정면 기준 이 각도(도) 안쪽의 지점을 우선 선택 — 매번 뒤돌아 왔다갔다 하는 것 방지
	UPROPERTY(EditAnywhere, Category = "Wander|Smoothing", meta = (ClampMin = "10.0", ClampMax = "180.0"))
	float preferredTurnAngle = 110.f;

	// 배회 중 가속도 (cm/s²) — 기본값(2048)은 한 프레임 만에 최고 속도라 출발이 뚝 끊겨 보임
	UPROPERTY(EditAnywhere, Category = "Wander|Smoothing", meta = (ClampMin = "50.0"))
	float wanderAcceleration = 250.f;

	// 도착 지점 앞 이 거리(cm)부터 감속 — 급정지 방지
	UPROPERTY(EditAnywhere, Category = "Wander|Smoothing", meta = (ClampMin = "0.0"))
	float wanderBrakingDistance = 80.f;

	// 화면에 안 보이는 동안에는 새 이동을 시작하지 않고 대기만 함 (경로 탐색·이동 연산 절약)
	UPROPERTY(EditAnywhere, Category = "Wander|Optimization")
	bool bWanderOnlyWhenRendered = true;

private:
	bool HasTarget(UBehaviorTreeComponent& OwnerComp) const;
	bool PickWanderPoint(const class AC_BaseMonster* Monster, FVector& OutPoint) const;
	bool StartTurn(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);
	bool StartMove(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);
};
