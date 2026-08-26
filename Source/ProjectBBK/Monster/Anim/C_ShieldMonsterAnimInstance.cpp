// Fill out your copyright notice in the Description page of Project Settings.

#include "C_ShieldMonsterAnimInstance.h"
#include "../C_ShieldMonster.h"

void UC_ShieldMonsterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	// 부모가 OwnerPawn 확보 + Speed / Dir 갱신을 담당
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!shieldMonster.IsValid())
		shieldMonster = Cast<AC_ShieldMonster>(OwnerPawn);

	// 데미지 차단 판정과 동일한 값을 쓴다 — 방패가 올라가 있는 동안만 막히고,
	// 노말 공격이 나가는 동안에는 내려가서 그대로 딜 타이밍이 된다.
	// 상체 방패 포즈는 additive라 하체 로코모션은 블렌드스페이스가 그대로 굴린다
	const float target = (shieldMonster.IsValid() && shieldMonster->IsGuarding()) ? 1.f : 0.f;

	GuardAlpha = guardBlendSpeed > 0.f
		? FMath::FInterpTo(GuardAlpha, target, DeltaSeconds, guardBlendSpeed)
		: target;
}
