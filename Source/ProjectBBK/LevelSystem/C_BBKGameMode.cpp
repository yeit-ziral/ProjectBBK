// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BBKGameMode.h"
#include "C_Portal.h"
#include "C_BBKGameInstance.h"
#include "../Monster/C_BaseMonster.h"
#include "../UI/C_MinimapWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
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

	if (ShouldShowMinimap())
	{
		TryCreateMinimap();
	}
}

void AC_BBKGameMode::TryCreateMinimap()
{
	UWorld* World = GetWorld();
	if (!World || minimapWidget) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController() || !PC->GetLocalPlayer())
	{
		// 아직 로컬 플레이어가 안 붙었다 — 이 상태로는 CreateWidget이 실패하므로 준비될 때까지 재시도
		if (!World->GetTimerManager().IsTimerActive(minimapRetryTimer))
		{
			World->GetTimerManager().SetTimer(minimapRetryTimer, this, &AC_BBKGameMode::TryCreateMinimap, 0.2f, true);
		}
		return;
	}

	World->GetTimerManager().ClearTimer(minimapRetryTimer);

	UClass* WidgetClass = minimapWidgetClass ? minimapWidgetClass.Get() : UC_MinimapWidget::StaticClass();
	minimapWidget = CreateWidget<UC_MinimapWidget>(PC, WidgetClass);
	if (!minimapWidget) return;

	// 우상단 고정. SetDesiredSizeInViewport/SetPositionInViewport는 내부에서 앵커를 (0,0)으로 되돌리므로
	// (Debugging Checklist #56) 슬롯을 직접 채워서 넘긴다. Offsets = (위치X, 위치Y, 폭, 높이)
	UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get(World);
	if (!ViewportSubsystem) return;

	FGameViewportWidgetSlot ViewportSlot;
	ViewportSlot.Anchors = FAnchors(1.f, 0.f);
	ViewportSlot.Alignment = FVector2D(1.f, 0.f);
	ViewportSlot.Offsets = FMargin(-minimapWidget->screenMargin.X, minimapWidget->screenMargin.Y, minimapWidget->mapSize, minimapWidget->mapSize);
	ViewportSlot.ZOrder = 0;
	ViewportSubsystem->AddWidget(minimapWidget, ViewportSlot);
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
