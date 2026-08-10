// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_ConsumableAction.h"
#include "C_KnockbackAction.generated.h"

/**
 * 사용 시 AvatarActor 주변 몬스터를 순수 물리적으로 밀어내는(넉백) 소비 아이템 액션.
 * GE 적용 없음 — LaunchCharacter만 수행 (C_RangedUltimate::HandleNotifyEvent 패턴 재사용).
 */
UCLASS(Blueprintable)
class PROJECTBBK_API UC_KnockbackAction : public UC_ConsumableAction
{
	GENERATED_BODY()

protected:
	virtual void Execute_Implementation(UAbilitySystemComponent* ASC, AActor* AvatarActor) override;

	UPROPERTY(EditDefaultsOnly, Category = "Knockback")
	float radius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Knockback")
	float knockbackForce = 800.f;
};
