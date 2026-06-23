// Fill out your copyright notice in the Description page of Project Settings.


#include "C_EliteMonster.h"
#include "M_Gas/C_MonsterASC.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AC_EliteMonster::AC_EliteMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AC_EliteMonster::BeginPlay()
{
	Super::BeginPlay();

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	monsterTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Monster.Type.Normal"));

	// 공격 GA를 ASC에 등록 — TryActivateAbilityByClass는 GiveAbility된 어빌리티만 발동 가능
	if (monsterASC)
	{
		if (normalAttackGAClass && !monsterASC->FindAbilitySpecFromClass(normalAttackGAClass))
			monsterASC->GiveAbility(FGameplayAbilitySpec(normalAttackGAClass, 1, 0));
		if (special1GAClass && !monsterASC->FindAbilitySpecFromClass(special1GAClass))
			monsterASC->GiveAbility(FGameplayAbilitySpec(special1GAClass, 1, 0));
		if (special2GAClass && !monsterASC->FindAbilitySpecFromClass(special2GAClass))
			monsterASC->GiveAbility(FGameplayAbilitySpec(special2GAClass, 1, 0));
	}
}

void AC_EliteMonster::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// [디버그] BT 없이 일정 간격으로 자동 공격 (스페셜 쿨다운 시 스페셜, 아니면 노말)
	if (!bDebugAutoAttack) return;

	debugAttackAccum += DeltaSeconds;
	if (debugAttackAccum >= debugAttackInterval)
	{
		debugAttackAccum = 0.f;
		EliteAutoAttack();
	}
}

bool AC_EliteMonster::CanNormalAttack() const
{
	if (!monsterASC) return false;
	return GetWorld()->GetTimeSeconds() - lastNormalAttackTime >= GetAttackCooldown();
}

bool AC_EliteMonster::CanSpecialAttack() const
{
	if (!monsterASC) return false;
	return GetWorld()->GetTimeSeconds() - lastSpecialAttackTime >= GetSpecialCooldown();
}

bool AC_EliteMonster::CanAutoAttack() const
{
	return CanNormalAttack() || CanSpecialAttack();
}

bool AC_EliteMonster::EliteAutoAttack()
{
	// 스페셜 쿨타임이 끝났으면 스페셜 우선, 아니면 노말
	if (CanSpecialAttack())
		return EliteSpecialAttack();
	return EliteNormalAttack();
}

bool AC_EliteMonster::EliteNormalAttack()
{
	if (!monsterASC || !normalAttackGAClass) return false;
	if (!CanNormalAttack()) return false;

	const bool bActivated = monsterASC->TryActivateAbilityByClass(normalAttackGAClass);
	if (bActivated)
		lastNormalAttackTime = GetWorld()->GetTimeSeconds();
	return bActivated;
}

bool AC_EliteMonster::EliteSpecialAttack()
{
	if (!monsterASC) return false;
	if (!CanSpecialAttack()) return false;

	// 교대 발동 — 한쪽만 할당된 경우 다른 쪽으로 폴백
	TSubclassOf<UGameplayAbility> ga = bNextSpecialIsSecond ? special2GAClass : special1GAClass;
	if (!ga)
		ga = bNextSpecialIsSecond ? special1GAClass : special2GAClass;
	if (!ga) return false;

	const bool bActivated = monsterASC->TryActivateAbilityByClass(ga);
	if (bActivated)
	{
		lastSpecialAttackTime = GetWorld()->GetTimeSeconds();
		bNextSpecialIsSecond  = !bNextSpecialIsSecond;
	}
	return bActivated;
}
