// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_ShieldParryWindow.generated.h"

/**
 * 쉴드 몬스터 노말 공격 몽타주에 얹는 패링 윈도우.
 * 노티파이 스테이트의 **끝**이 곧 실제 타격 시점이 되도록 배치할 것.
 *
 *   NotifyBegin → AC_ShieldMonster::BeginAttackTelegraph(TotalDuration)  : 링 표시 + 윈드업 정지 시작
 *   NotifyEnd   → AC_ShieldMonster::ResolveStrike()                      : 두 원이 겹치는 순간 판정
 *
 * 흰 원 수렴은 AC_ShieldMonster::Tick이 실시간으로 진행시킨다(NotifyTick 미사용) —
 * 이 구간은 감속 재생 후 원속 재생으로 배속이 바뀌므로 애니 시간과 실시간이 비례하지 않고,
 * NotifyTick 델타가 애니 시간인지 프레임 시간인지에 따라 이중 가산될 위험도 있기 때문.
 * BeginAttackTelegraph가 두 단계의 실시간 합으로 총 수렴 시간을 잡으므로
 * 배속을 바꿔도 링이 겹치는 순간과 타격 판정은 어긋나지 않는다.
 *
 * **주의**: 감속 배속에 0을 넣으면 안 된다. 애니 델타가 0이면 UAnimSequenceBase::GetAnimNotifies가
 * 아무것도 수집하지 않아 UE가 이 NotifyState를 강제 종료하고 타격 판정이 반복 실행된다.
 */
UCLASS()
class PROJECTBBK_API UANS_ShieldParryWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd  (USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
