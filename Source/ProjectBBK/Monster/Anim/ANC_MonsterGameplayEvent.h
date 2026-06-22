// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "ANC_MonsterGameplayEvent.generated.h"

/**
 * 범용 몬스터 AnimNotify — 지정한 eventTag GameplayEvent를 몬스터(소유 액터)에게 전송한다.
 * 태그가 하드코딩된 ANC_MeleeNormalAttack/Slam과 달리 에디터에서 태그를 지정해 재사용한다.
 * 예) 엘리트 스페셜2 Cast 애님의 던지는 프레임에 배치 → Event.Monster.Elite.ThrowRelease.
 */
UCLASS(meta = (DisplayName = "Monster Gameplay Event"))
class PROJECTBBK_API UANC_MonsterGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	// 전송할 GameplayEvent 태그 (에디터에서 지정)
	UPROPERTY(EditAnywhere, Category = "Monster")
	FGameplayTag eventTag;
};
