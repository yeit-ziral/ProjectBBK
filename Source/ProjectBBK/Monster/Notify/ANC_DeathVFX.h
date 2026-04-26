// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ANC_DeathVFX.generated.h"

/**
 * Death 몽타주 내 지정된 프레임에서 VFX를 트리거하는 AnimNotify.
 * 몽타주 에디터에서 VFX 발동 타이밍을 직접 조절 가능.
 */
UCLASS()
class PROJECTBBK_API UANC_DeathVFX : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override { return TEXT("DeathVFX"); }

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
