// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MeleeMonster.h"
#include "Manager/C_AttackManagerComponent.h"
#include "Data/AttackTypes.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	monsterTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Monster.Type.Normal"));

	//MeleeSpecialAttack();
	//MeleeNormalAttack();
}

void AC_MeleeMonster::MeleeAutoAttack()
{
	if (!attackManager) return;

	if (attackManager->CanSpecialAttack())
	{
		MeleeSpecialAttack();
	}
	else
	{
		MeleeNormalAttack();
	}
}

void AC_MeleeMonster::MeleeNormalAttack()
{
	if (!attackManager)
		return;

	if (attackManager->DoNormalAttack())
	{
		PlayAnimMontage(meleeNormalMontage);
	}

}

void AC_MeleeMonster::MeleeSpecialAttack()
{
	if (!attackManager) return;

	if (attackManager->DoSpecialAttack())
	{
		PlayAnimMontage(meleeSpecialMontage);
	}
}
