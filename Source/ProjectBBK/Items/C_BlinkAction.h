// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_ConsumableAction.h"
#include "C_BlinkAction.generated.h"

/**
 * 사용 시 AvatarActor를 전방(ForwardVector)으로 즉시 순간이동시키는 소비 아이템 액션.
 * GE 적용 없음 — LineTrace로 벽/장애물을 체크해 통과하지 않도록 거리를 단축한 뒤 SetActorLocation만 수행.
 */
UCLASS(Blueprintable)
class PROJECTBBK_API UC_BlinkAction : public UC_ConsumableAction
{
	GENERATED_BODY()

protected:
	virtual void Execute_Implementation(UAbilitySystemComponent* ASC, AActor* AvatarActor) override;

	UPROPERTY(EditDefaultsOnly, Category = "Blink")
	float blinkDistance = 600.f;
};
