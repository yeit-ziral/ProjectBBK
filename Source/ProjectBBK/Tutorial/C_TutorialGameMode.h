// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../LevelSystem/C_BBKGameMode.h"
#include "C_TutorialGameMode.generated.h"

class UDataTable;
class UC_TutorialPromptWidget;
class AC_PlayerController;

/**
 * 튜토리얼 레벨 전용 GameMode.
 * AC_BBKGameMode를 그대로 상속하되(포탈 수집·레벨 전환 로직 재사용) 포탈 자동 활성화만 막고,
 * 튜토리얼 전 단계를 마쳤을 때 직접 연다.
 *
 * 에셋 참조를 여기에 두어 BPC_PlayerController를 건드리지 않는다 —
 * UC_TutorialComponent는 순수 진행 기계로 남는다.
 *
 * 시작 시점 주의: 메인 메뉴에서 OpenLevel로 넘어오는 경로에서는 GameMode의 BeginPlay가
 * 로컬 플레이어가 붙기 전에 실행된다. 그 시점의 PlayerController로는 CreateWidget도,
 * InputComponent 바인딩도 할 수 없으므로 준비될 때까지 기다렸다 시작한다.
 */
UCLASS()
class PROJECTBBK_API AC_TutorialGameMode : public AC_BBKGameMode
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 플레이어가 실제로 들어온 시점 — 레벨 이동 경로에서는 BeginPlay보다 늦게 호출된다
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	// 몬스터 전멸로 포탈이 열리면 안 된다 — 튜토리얼 완료가 유일한 조건
	virtual bool ShouldAutoActivatePortals() const override { return false; }

	// BP_TutorialGameMode에서 DT_TutorialSteps 할당
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	UDataTable* tutorialStepTable;

	// BP_TutorialGameMode에서 WBP_TutorialPrompt 할당
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	TSubclassOf<UC_TutorialPromptWidget> promptWidgetClass;

	// 플레이어가 준비될 때까지 다시 확인하는 주기 (초)
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial", meta = (ClampMin = "0.01"))
	float startRetryInterval = 0.1f;

	// 이 시간이 지나도 준비되지 않으면 포기하고 에러를 남긴다 (무한 타이머 방지)
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial", meta = (ClampMin = "0.0"))
	float startTimeout = 10.0f;

private:
	UFUNCTION()
	void OnTutorialCompleted_Handler(int32 CompletedStepCount);

	// 준비된 PlayerController를 찾으면 튜토리얼을 시작하고, 아니면 재시도 타이머를 건다.
	// BeginPlay / HandleStartingNewPlayer / 재시도 타이머 세 경로에서 모두 호출된다.
	void TryStartTutorial();

	// CreateWidget과 입력 바인딩이 모두 가능한 상태의 로컬 PlayerController만 돌려준다.
	// 조건 미달이면 nullptr.
	AC_PlayerController* FindReadyPlayerController() const;

	FTimerHandle startRetryTimer;

	// 중복 시작 방지 — 세 경로에서 호출되므로 반드시 필요
	bool bTutorialStarted = false;

	float startWaitElapsed = 0.f;
};
