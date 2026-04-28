// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectBBK/GAS/Abilities/C_CharacterGA.h"
#include "GA_RangeAttack.generated.h"

class AC_PlayerRangedProjectile;
class UGameplayEffect;

/**
 * 원거리 캐릭터 기본 공격.
 *
 * 흐름:
 *  1. ActivateAbility → CommitAbility → 몽타주 재생
 *  2. AnimNotify에서 Event.Montage.FireProjectile 태그 전송
 *  3. OnFireProjectile → 카메라 전방 LineTrace로 조준점 계산
 *                      → ProjectileSocket에서 투사체 스폰 → EndAbility
 *
 * Blueprint 설정:
 *  - AttackMontage      : 공격 애니메이션 몽타주
 *  - ProjectileClass    : AC_PlayerRangedProjectile (또는 자식 BP)
 *  - DamageEffectClass  : GE_BasicDamage
 *  - BaseDamage         : 기본 데미지 수치
 *  - ProjectileSocketName : 발사 위치로 사용할 스켈레탈 메시 소켓 이름
 */
UCLASS()
class PROJECTBBK_API UGA_RangeAttack : public UC_CharacterGA
{
	GENERATED_BODY()

public:
	UGA_RangeAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnFireProjectile(FGameplayEventData Payload);

	// ── Blueprint에서 설정할 프로퍼티 ──────────────────────────────

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
