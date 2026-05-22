// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectBBK/GAS/Abilities/C_CharacterGA.h"
#include "C_RangeAttackGA.generated.h"

class AC_PlayerRangedProjectile;
class UGameplayEffect;

UCLASS()
class PROJECTBBK_API UC_RangeAttackGA : public UC_CharacterGA
{
	GENERATED_BODY()

public:
	UC_RangeAttackGA();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "RangeAttack|Animation")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "RangeAttack|Projectile")
	TSubclassOf<AC_PlayerRangedProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "RangeAttack|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "RangeAttack|Damage")
	float BaseDamage = 20.0f;

	/** 스켈레탈 메시에서 투사체를 발사할 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "RangeAttack|Projectile")
	FName ProjectileSocketName = TEXT("WeaponSocket");

	/** 카메라 전방 조준 LineTrace 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "RangeAttack|Projectile")
	float AimTraceDistance = 10000.0f;
};
