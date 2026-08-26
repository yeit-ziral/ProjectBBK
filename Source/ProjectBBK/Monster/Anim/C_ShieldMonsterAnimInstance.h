// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_MonsterAnimInstance.h"
#include "C_ShieldMonsterAnimInstance.generated.h"

class AC_ShieldMonster;

/**
 * 쉴드 몬스터 전용 AnimInstance.
 * 부모(UC_MonsterAnimInstance)가 채우는 Speed / Dir에 더해, 가드 포즈 블렌딩 값을 계산한다.
 *
 * Terra 팩에 막기 전용 모션이 없어 RMB_Shield_Jog_pose_MSA(Mesh Space Additive)를
 * 로코모션 위에 덧씌워 가드 모션을 대신한다.
 * 단 **상시가 아니라 실제로 공격을 막은 순간에만** 올린다(AC_ShieldMonster::IsBlockPoseActive).
 * 평소엔 방패를 내리고 걸어다니다가 피격 순간 방패가 올라오는 연출.
 * AnimGraph 구성:
 *
 *   [Blendspace Player] ──► Base     ┐
 *                                     ├─ [Apply Mesh Space Additive] ─► [Slot] ─► [Output Pose]
 *   [Sequence Player]   ──► Additive ┘        Alpha ◄── GuardAlpha
 *    RMB_Shield_Jog_pose_MSA
 *
 * Additive는 반드시 Slot보다 **앞**에 둘 것 — 뒤에 두면 공격·사망 몽타주에도 방패 팔 오프셋이 섞인다.
 */
UCLASS()
class PROJECTBBK_API UC_ShieldMonsterAnimInstance : public UC_MonsterAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Guard")
	float GetGuardAlpha() const { return GuardAlpha; }

protected:
	// Apply Mesh Space Additive의 Alpha 핀에 바인딩. AC_ShieldMonster::IsGuarding()을 보간한 값
	UPROPERTY(BlueprintReadOnly, Category = "Guard", meta = (AllowPrivateAccess = "true"))
	float GuardAlpha = 0.f;

	// 가드를 올리고 내리는 보간 속도. 0이면 보간 없이 즉시 전환(방패가 툭 튐)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard", meta = (ClampMin = "0.0"))
	float guardBlendSpeed = 10.f;

private:
	TWeakObjectPtr<AC_ShieldMonster> shieldMonster;
};
