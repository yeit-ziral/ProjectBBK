// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MonsterDataComponent.h"
#include "../C_BaseMonster.h"
#include "../Data/MonsterData.h"
#include "../M_Gas/C_MonsterAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"

UC_MonsterDataComponent::UC_MonsterDataComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UC_MonsterDataComponent::Initialize(AC_BaseMonster* InOwner)
{
	ownerMonster = InOwner;
	if (!ownerMonster) return;

	UC_MonsterAttributeSet* attrSet = ownerMonster->GetMonsterAttributeSet();
	if (!attrSet) return;

	UDataTable* Table = monsterTable.Get();
	if (!Table)
		Table = monsterTable.LoadSynchronous();

	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataComponent] DataTable not loaded! (%s)"), *ownerMonster->GetName());
		return;
	}

	const FMonsterData* Data = Table->FindRow<FMonsterData>(rowName, TEXT("InitMonster"));
	if (!Data)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataComponent] Row '%s' not found in DataTable"), *rowName.ToString());
		return;
	}

	monsterId = Data->MonsterId;

	attrSet->InitmaxHP          (Data->MaxHP);
	attrSet->InitmaxGroggy      (Data->MaxGroggy);
	attrSet->Initattack         (Data->Attack);
	attrSet->Initdefense        (Data->Defense);
	attrSet->InitattackRange    (Data->AttackRange);
	attrSet->InitmoveSpeed      (Data->MoveSpeed);
	attrSet->InitnormalCooldown (Data->NormalCooldown);
	attrSet->InitspecialCooldown(Data->SpecialCooldown);

	attrSet->SetcurHP(Data->MaxHP);
	attrSet->SetcurGroggy(0.0f);

	if (UCharacterMovementComponent* Move = ownerMonster->GetCharacterMovement())
	{
		Move->MaxWalkSpeed = attrSet->GetmoveSpeed();
	}
}
