// Fill out your copyright notice in the Description page of Project Settings.


#include "C_EliteMonster.h"
#include "M_Gas/C_MonsterASC.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

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

bool AC_EliteMonster::IsPlayingAttackAnimation() const
{
	return IsAbilityActive(normalAttackGAClass)
		|| IsAbilityActive(special1GAClass)
		|| IsAbilityActive(special2GAClass);
}

float AC_EliteMonster::GetDistanceToPlayer() const
{
	const APawn* player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!player) return FLT_MAX;
	return FVector::Dist(player->GetActorLocation(), GetActorLocation());
}

bool AC_EliteMonster::IsSpecialInRange(bool bSecond) const
{
	const float range = bSecond ? GetSpecial2Range() : GetSpecial1Range();
	return GetDistanceToPlayer() <= range;
}

bool AC_EliteMonster::IsSpecialOffCooldown(bool bSecond) const
{
	// 두 스페셜은 쿨다운을 공유하지 않는다 — 슬롯마다 마지막 발동 시각·쿨다운을 따로 본다
	const float lastTime = bSecond ? lastSpecial2AttackTime : lastSpecial1AttackTime;
	const float cooldown = bSecond ? GetSpecial2Cooldown()  : GetSpecial1Cooldown();
	return GetAttackClock() - lastTime >= cooldown;
}

bool AC_EliteMonster::CanUseSpecial(bool bSecond) const
{
	const TSubclassOf<UGameplayAbility> ga = bSecond ? special2GAClass : special1GAClass;
	if (!ga) return false;
	return IsSpecialOffCooldown(bSecond) && IsSpecialInRange(bSecond);
}

bool AC_EliteMonster::CanNormalAttack() const
{
	if (!monsterASC) return false;
	if (GetAttackClock() - lastNormalAttackTime < GetAttackCooldown()) return false;
	// 노말은 근접 판정(전방 구체 트레이스)이므로 AttackRange 안에서만 발동
	return GetDistanceToPlayer() <= GetAttackRange();
}

bool AC_EliteMonster::CanSpecialAttack() const
{
	if (!monsterASC) return false;
	// 슬롯마다 쿨다운·사거리가 독립 — 하나라도 발동 가능하면 true
	return CanUseSpecial(false) || CanUseSpecial(true);
}

bool AC_EliteMonster::CanAutoAttack() const
{
	return CanNormalAttack() || CanSpecialAttack();
}

bool AC_EliteMonster::EliteAutoAttack()
{
	// 스페셜이 쿨타임·사거리를 모두 만족하면 우선, 실패하면 노말로 폴백
	if (CanSpecialAttack() && EliteSpecialAttack())
		return true;
	return EliteNormalAttack();
}

bool AC_EliteMonster::EliteNormalAttack()
{
	if (!monsterASC || !normalAttackGAClass) return false;
	if (!CanNormalAttack()) return false;

	const bool bActivated = monsterASC->TryActivateAbilityByClass(normalAttackGAClass);
	if (bActivated)
		lastNormalAttackTime = GetAttackClock();
	return bActivated;
}

bool AC_EliteMonster::EliteSpecialAttack()
{
	if (!monsterASC) return false;
	if (!CanSpecialAttack()) return false;

	// 둘 다 발동 가능하면 교대 플래그로 우선순위를 번갈아 주고,
	// 한쪽만 가능하면(쿨다운 중이거나 사거리 밖) 가능한 쪽이 그대로 나간다
	const bool preferSecond = bNextSpecialIsSecond;
	const bool tryOrder[2] = { preferSecond, !preferSecond };

	for (const bool bSecond : tryOrder)
	{
		if (!CanUseSpecial(bSecond)) continue;

		TSubclassOf<UGameplayAbility> ga = bSecond ? special2GAClass : special1GAClass;
		if (monsterASC->TryActivateAbilityByClass(ga))
		{
			// 발동한 슬롯의 쿨다운만 갱신 — 다른 슬롯은 영향 없음
			(bSecond ? lastSpecial2AttackTime : lastSpecial1AttackTime) = GetAttackClock();
			// 이번에 쓴 슬롯의 반대쪽을 다음 차례로
			bNextSpecialIsSecond = !bSecond;
			return true;
		}
	}
	return false;
}
