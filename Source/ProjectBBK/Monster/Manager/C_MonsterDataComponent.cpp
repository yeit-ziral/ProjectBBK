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

	const float finalMaxHP = Data->MaxHP + ownerMonster->level * 50.0f;

	attrSet->InitmaxHP          (finalMaxHP);
	attrSet->InitmaxGroggy      (Data->MaxGroggy);
	attrSet->Initattack         (Data->Attack);
	attrSet->Initdefense        (Data->Defense);
	attrSet->InitattackRange    (Data->AttackRange);
	attrSet->InitmoveSpeed      (Data->MoveSpeed);
	attrSet->InitnormalCooldown (Data->NormalCooldown);
	attrSet->InitspecialCooldown(Data->SpecialCooldown);


	attrSet->SetcurHP(finalMaxHP);
	attrSet->SetcurGroggy(0.0f);

	bEnableReposition      = Data->bEnableReposition;
	repositionDesiredRange = Data->RepositionDesiredRange;
	repositionMinRange     = Data->RepositionMinRange;
	repositionSpeed        = Data->RepositionSpeed;
	repositionStrafeWeight = Data->RepositionStrafeWeight;
	repositionBand         = Data->RepositionBand;
	repositionFlipInterval = Data->RepositionFlipInterval;
	expReward              = Data->ExpReward;

	// 스페셜 사거리 미지정(0 이하)이면 노말 사거리와 동일하게 취급
	special1Range = (Data->Special1Range > 0.f) ? Data->Special1Range : Data->AttackRange;
	special2Range = (Data->Special2Range > 0.f) ? Data->Special2Range : Data->AttackRange;

	// 슬롯별 쿨다운 미지정(0 이하)이면 공유 SpecialCooldown 사용
	special1Cooldown = (Data->Special1Cooldown > 0.f) ? Data->Special1Cooldown : Data->SpecialCooldown;
	special2Cooldown = (Data->Special2Cooldown > 0.f) ? Data->Special2Cooldown : Data->SpecialCooldown;

	if (UCharacterMovementComponent* Move = ownerMonster->GetCharacterMovement())
	{
		Move->MaxWalkSpeed = attrSet->GetmoveSpeed();
	}
}
