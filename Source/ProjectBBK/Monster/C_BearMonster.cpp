// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BearMonster.h"
#include "Manager/C_AttackManagerComponent.h" 
#include "Data/AttackTypes.h" 

AC_BearMonster::AC_BearMonster()
{
	rowName = "Bear";

}

void AC_BearMonster::BearNormalAttack()
{
	if (!attackManager)
		return;

	UE_LOG(LogTemp, Warning, TEXT("BearMonster 공격 시도"));

	//if (!bearNormalMontage)
	//	UE_LOG(LogTemp, Warning, TEXT("no nomal montage"))
	//else if (bearNormalMontage)
	//	UE_LOG(LogTemp, Warning, TEXT("yes nomal montage"));

	//PlayAnimMontage(bearNormalMontage);

	if (attackManager->DoNormalAttack())
	{
			if(!bearNormalMontage)
			UE_LOG(LogTemp, Warning, TEXT("no nomal montage"))
			else if(bearNormalMontage)
			UE_LOG(LogTemp, Warning, TEXT("yes nomal montage"));
	
		PlayAnimMontage(bearNormalMontage);
	}
	
}

void AC_BearMonster::BearSpecialAttack()
{
	UE_LOG(LogTemp, Warning, TEXT("BearSpecialAttack 호출됨 — attackManager = %s"),
		attackManager ? TEXT("VALID") : TEXT("NULL"));
	
	if (!attackManager)
		return;

	//PlayAnimMontage(bearSpecialMontage);

	UE_LOG(LogTemp, Warning, TEXT("BearMonster 특수 공격 시도"));

	if (attackManager->DoSpecialAttack())
	{
		if(!bearSpecialMontage)
			UE_LOG(LogTemp, Warning, TEXT("no montage"));
		PlayAnimMontage(bearSpecialMontage);
	}

}

void AC_BearMonster::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("BearMonster BeginPlay 실행됨 - HP=%d, ATK=%d, Speed=%.1f, AttackRange=%.1f"),
		                           hp, attack, moveSpeed, attackRange);

	//BearSpecialAttack();
	//BearNormalAttack();
}
