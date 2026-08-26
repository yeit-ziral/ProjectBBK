// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "C_BTTaskMeleeAutoAttack.generated.h"

/**
 * 몬스터 공통 자동공격 BT 태스크.
 *
 * 원래 근접 몬스터 전용이었으나 AC_BaseMonster::TryAutoAttack() 가상 함수를 경유하도록 일반화해서
 * 모든 AC_BaseMonster 파생 몬스터에 그대로 쓸 수 있다 (쉴드 몬스터가 이걸 그대로 사용).
 * 새 몬스터를 추가할 때 BT Task 클래스를 새로 만들 필요 없이 TryAutoAttack()만 override하면 된다.
 *
 * 클래스명이 Melee로 남아 있는 이유: 기존 BT 에셋(BT_Monster_Melee / Shield)이 이 클래스 경로를
 * 참조하고 있어 이름을 바꾸면 노드가 끊어짐. 표시 이름(NodeName)만 "Monster Auto Attack"으로 변경.
 *
 * 쿨다운 중이면 Failed를 반환해 Selector가 UC_BTTaskReposition fallback으로 넘어가게 한다.
 */
UCLASS()
class PROJECTBBK_API UC_BTTaskMeleeAutoAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UC_BTTaskMeleeAutoAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
