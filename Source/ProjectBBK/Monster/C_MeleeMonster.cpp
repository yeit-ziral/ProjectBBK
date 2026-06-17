// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MeleeMonster.h"
#include "Manager/C_AttackManagerComponent.h"
#include "Data/AttackTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"

AC_MeleeMonster::AC_MeleeMonster()
{
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AC_MeleeMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AC_MeleeMonster::BeginPlay()
{
	Super::BeginPlay();

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	monsterTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Monster.Type.Normal"));

	//MeleeSpecialAttack();
	//MeleeNormalAttack();
}

bool AC_MeleeMonster::IsPlayingAttackAnimation() const
{
	UAnimInstance* anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!anim) return false;
	return (meleeNormalMontage  && anim->Montage_IsPlaying(meleeNormalMontage))
		|| (meleeSpecialMontage && anim->Montage_IsPlaying(meleeSpecialMontage));
}

bool AC_MeleeMonster::CanAutoAttack() const
{
	if (!attackManager) return false;
	return attackManager->CanAttack() || attackManager->CanSpecialAttack();
}

bool AC_MeleeMonster::MeleeAutoAttack()
{
	if (!attackManager) return false;

	const bool bCanSpecial = attackManager->CanSpecialAttack();
	UE_LOG(LogTemp, Warning, TEXT("[Melee] AutoAttack — CanSpecial=%s  CanNormal=%s"),
		bCanSpecial ? TEXT("true") : TEXT("false"),
		attackManager->CanAttack() ? TEXT("true") : TEXT("false"));

	if (bCanSpecial)
		return MeleeSpecialAttack();
	else
		return MeleeNormalAttack();
}

bool AC_MeleeMonster::MeleeNormalAttack()
{
	if (!attackManager || !monsterASC || !normalAttackGAClass) return false;
	if (!attackManager->DoNormalAttack()) return false;

	return monsterASC->TryActivateAbilityByClass(normalAttackGAClass);
}

bool AC_MeleeMonster::MeleeSpecialAttack()
{
	if (!attackManager || !monsterASC || !specialAttackGAClass) return false;
	if (!attackManager->DoSpecialAttack()) return false;

	return monsterASC->TryActivateAbilityByClass(specialAttackGAClass);
}
