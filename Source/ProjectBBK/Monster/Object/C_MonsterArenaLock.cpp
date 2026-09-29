// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MonsterArenaLock.h"
#include "../C_BaseMonster.h"
#include "../M_Gas/C_MonsterASC.h"
#include "../../Items/C_BaseItem.h"
#include "Components/BillboardComponent.h"

AC_MonsterArenaLock::AC_MonsterArenaLock()
{
	PrimaryActorTick.bCanEverTick = false;

	editorIcon = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorIcon"));
	RootComponent = editorIcon;
	editorIcon->bIsEditorOnly = true;
}

void AC_MonsterArenaLock::BeginPlay()
{
	Super::BeginPlay();

	// 상자 잠금 — 콜리전을 꺼두면 BeginOverlap이 안 들어와 상호작용 UI/입력 등록이 안 된다
	for (AC_BaseItem* Chest : lockedChests)
	{
		if (Chest) Chest->SetActorEnableCollision(false);
	}

	for (AC_BaseMonster* Monster : guardMonsters)
	{
		if (!Monster) continue;

		UC_MonsterASC* ASC = Monster->GetMonsterASC();
		if (!ASC || aliveASCs.Contains(ASC)) continue;

		aliveASCs.Add(ASC);
		ASC->OnMonsterDeath.AddUObject(this, &AC_MonsterArenaLock::HandleMonsterDeath);
	}

	remainingMonsterCount = aliveASCs.Num();

	// 몬스터 지정이 비어 있으면 잠글 이유가 없음
	if (remainingMonsterCount <= 0)
	{
		Unlock();
	}
}

void AC_MonsterArenaLock::HandleMonsterDeath(UC_MonsterASC* DeadASC)
{
	if (bCleared || !aliveASCs.Remove(DeadASC)) return;

	remainingMonsterCount = aliveASCs.Num();
	if (remainingMonsterCount <= 0)
	{
		Unlock();
	}
}

void AC_MonsterArenaLock::Unlock()
{
	if (bCleared) return;
	bCleared = true;

	for (AC_BaseItem* Chest : lockedChests)
	{
		if (!Chest) continue;

		Chest->SetActorEnableCollision(true);
		// 플레이어가 이미 상자 앞에 서 있으면 BeginOverlap이 다시 안 오므로 현재 오버랩을 재반영
		Chest->RefreshOverlapState();
	}

	OnArenaCleared.Broadcast(this);
}
