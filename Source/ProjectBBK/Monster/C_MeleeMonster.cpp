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
	if (!attackManager) return false;

	if (attackManager->DoNormalAttack())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Melee] NormalAttack — Montage=%s"),
			meleeNormalMontage ? *meleeNormalMontage->GetName() : TEXT("NULL"));
		float len = PlayAnimMontage(meleeNormalMontage);
		UE_LOG(LogTemp, Warning, TEXT("[Melee] PlayAnimMontage returned %.3f"), len);
		return true;
	}
	return false;
}

bool AC_MeleeMonster::MeleeSpecialAttack()
{
	if (!attackManager) return false;

	if (attackManager->DoSpecialAttack())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Melee] SpecialAttack — Montage=%s  SpecialCooldown=%.2f"),
			meleeSpecialMontage ? *meleeSpecialMontage->GetName() : TEXT("NULL"),
			GetSpecialCooldown());
		float len = PlayAnimMontage(meleeSpecialMontage);
		UE_LOG(LogTemp, Warning, TEXT("[Melee] SpecialMontage played, length=%.3f"), len);
		// 데미지는 ANC_MeleeSpecialAttack AnimNotify에서만 처리
		return true;
	}
	return false;
}
