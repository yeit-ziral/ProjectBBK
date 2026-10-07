// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "C_BBKGameMode.generated.h"

class AC_Portal;
class UC_MinimapWidget;

// 레벨 몬스터 전멸 순간 브로드캐스트 — 수신 측이 RegisterPendingMonster()로 추가 몬스터를 예약하면 포탈 활성화가 미뤄진다
DECLARE_MULTICAST_DELEGATE(FOnLevelMonstersDefeated);

// 포탈이 실제로 열린 순간(= 레벨 클리어 확정) 브로드캐스트 — 클리어 보상 상인 등장 등에 사용
DECLARE_MULTICAST_DELEGATE(FOnLevelPortalsActivated);

UCLASS()
class PROJECTBBK_API AC_BBKGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AC_BBKGameMode();

	// C_BaseMonster::ExecuteDeathSequence()에서 호출 — 남은 몬스터 수 감소 및 포탈 활성화 체크
	void NotifyMonsterDead();

	// 레벨 시작 이후 스폰될 몬스터를 미리 집계에 포함 — 그 몬스터가 죽어야(NotifyMonsterDead) 포탈이 열린다.
	// OnAllMonstersDefeated 수신 중 호출하면 이번 전멸 시점의 포탈 활성화를 막을 수 있음
	void RegisterPendingMonster() { RemainingMonsterCount++; }

	FOnLevelMonstersDefeated OnAllMonstersDefeated;

	FOnLevelPortalsActivated OnPortalsActivated;

	// 이미 포탈이 열렸는지 — OnPortalsActivated보다 늦게 BeginPlay된 액터가 놓친 이벤트를 보정할 때 사용
	bool ArePortalsActivated() const { return bPortalsActivated; }

protected:
	virtual void BeginPlay() override;

	// 몬스터 전멸만으로 포탈을 열어도 되는지. 튜토리얼처럼 별도 완료 조건이 있는 레벨은 false를 반환하고
	// 자체 타이밍에 ActivateAllPortals()를 직접 호출한다.
	virtual bool ShouldAutoActivatePortals() const { return true; }

	void ActivateAllPortals();

	// 이 레벨에서 미니맵을 띄울지. 튜토리얼처럼 우상단을 다른 UI가 쓰는 레벨은 false
	virtual bool ShouldShowMinimap() const { return true; }

	// 비워 두면 UC_MinimapWidget 네이티브 클래스를 그대로 사용 (WBP 불필요)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UC_MinimapWidget> minimapWidgetClass;

private:
	UFUNCTION()
	void OnPortalEntered_Handler(AC_Portal* Portal);

	// 로컬 플레이어가 붙은 PlayerController가 준비되면 미니맵을 만든다.
	// 메인 메뉴에서 넘어오는 경로에서는 BeginPlay 시점에 아직 준비되지 않아 타이머로 재시도
	void TryCreateMinimap();

	UPROPERTY(Transient)
	TObjectPtr<UC_MinimapWidget> minimapWidget;

	FTimerHandle minimapRetryTimer;

	TArray<TWeakObjectPtr<AC_Portal>> RegisteredPortals;
	int32 RemainingMonsterCount = 0;
	bool bPortalsActivated = false;
};
