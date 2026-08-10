// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_ConsumableAction.h"
#include "C_SpawnHealZoneAction.generated.h"

class AC_HealZone;
class UGameplayEffect;

/**
 * 사용 시 AvatarActor 발밑에 AC_HealZone을 스폰하는 소비 아이템 액션.
 * 지면 위치는 AvatarActor 위치에서 아래로 LineTrace해 탐색한다
 * (3인칭 카메라 지면 위치 탐색 패턴 — docs/patterns.md 참고, 실패 시 스폰하지 않음).
 */
UCLASS(Blueprintable)
class PROJECTBBK_API UC_SpawnHealZoneAction : public UC_ConsumableAction
{
	GENERATED_BODY()

protected:
	virtual void Execute_Implementation(UAbilitySystemComponent* ASC, AActor* AvatarActor) override;

	UPROPERTY(EditDefaultsOnly, Category = "HealZone")
	TSubclassOf<AC_HealZone> zoneClass;

	UPROPERTY(EditDefaultsOnly, Category = "HealZone")
	TSubclassOf<UGameplayEffect> tickEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "HealZone")
	float radius = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "HealZone")
	float zoneLifetime = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "HealZone")
	float tickInterval = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "HealZone")
	float healPerTick = 5.f;
};
