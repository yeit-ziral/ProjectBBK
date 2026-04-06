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
	// TODO: 테스트용 그로기 누적 비활성화 — 정식 수치 확정 후 Super::Tick(DeltaTime)으로 교체
	ACharacter::Tick(DeltaTime);
	UpdateHpWidgetRotation();
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

	// TODO: 스페셜 공격 구현 후 CanSpecialAttack() 체크 추가
	MeleeNormalAttack();
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
	// TODO: 스페셜 공격 미구현
}
