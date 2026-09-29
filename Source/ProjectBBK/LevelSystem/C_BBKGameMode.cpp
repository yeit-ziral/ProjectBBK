// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BBKGameMode.h"
#include "C_Portal.h"
#include "C_BBKGameInstance.h"
#include "../Monster/C_BaseMonster.h"
#include "EngineUtils.h"

AC_BBKGameMode::AC_BBKGameMode()
{
	// 플레이어 컨트롤러가 직접 캐릭터를 스폰·빙의하므로 기본 Pawn 자동 스폰 비활성화
	DefaultPawnClass = nullptr;
}

void AC_BBKGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 레벨 내 몬스터 수 집계
	for (TActorIterator<AC_BaseMonster> It(GetWorld()); It; ++It)
	{
		RemainingMonsterCount++;
	}

	// 포탈 수집 및 이벤트 바인딩
	for (TActorIterator<AC_Portal> It(GetWorld()); It; ++It)
	{
		AC_Portal* Portal = *It;
		RegisteredPortals.Add(Portal);
		Portal->OnPortalEntered.AddDynamic(this, &AC_BBKGameMode::OnPortalEntered_Handler);
	}

	// 몬스터가 없는 레벨은 즉시 포탈 활성화
	if (RemainingMonsterCount <= 0 && ShouldAutoActivatePortals())
	{
		ActivateAllPortals();
	}
}

void AC_BBKGameMode::NotifyMonsterDead()
{
	RemainingMonsterCount--;
	if (RemainingMonsterCount > 0) return;

	// 전멸 알림 — 수신 측(AC_EliteSummonCircle 등)이 RegisterPendingMonster()로 추가 몬스터를 예약할 수 있음
	OnAllMonstersDefeated.Broadcast();

	if (RemainingMonsterCount <= 0 && ShouldAutoActivatePortals())
	{
		ActivateAllPortals();
	}
}

void AC_BBKGameMode::ActivateAllPortals()
{
	const bool bFirstActivation = !bPortalsActivated;
	bPortalsActivated = true;

	for (TWeakObjectPtr<AC_Portal>& PortalPtr : RegisteredPortals)
	{
		if (PortalPtr.IsValid())
		{
			PortalPtr->ActivatePortal();
		}
	}

	if (bFirstActivation)
	{
		OnPortalsActivated.Broadcast();
	}
}

void AC_BBKGameMode::OnPortalEntered_Handler(AC_Portal* Portal)
{
	if (UC_BBKGameInstance* GI = Cast<UC_BBKGameInstance>(GetGameInstance()))
	{
		GI->TravelToNextLevel();
	}
}
