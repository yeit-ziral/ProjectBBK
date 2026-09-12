// Fill out your copyright notice in the Description page of Project Settings.

#include "C_TutorialGameMode.h"
#include "C_TutorialComponent.h"
#include "../PlayerCharacter/PlayerAI/C_PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

void AC_TutorialGameMode::BeginPlay()
{
	// 포탈 수집·바인딩은 베이스가 처리. ShouldAutoActivatePortals() = false라 자동으로 열리지는 않는다.
	Super::BeginPlay();

	// PIE로 이 레벨을 바로 실행한 경우엔 이 시점에 이미 준비가 끝나 있어 즉시 시작된다.
	// 메인 메뉴에서 넘어온 경우엔 아직이라 재시도 타이머가 걸린다.
	TryStartTutorial();
}

void AC_TutorialGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(startRetryTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void AC_TutorialGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	// 레벨 이동 경로에서는 여기가 BeginPlay보다 늦다 — 타이머를 기다리지 않고 바로 시도한다
	TryStartTutorial();
}

void AC_TutorialGameMode::TryStartTutorial()
{
	if (bTutorialStarted)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AC_PlayerController* PC = FindReadyPlayerController();
	if (!PC)
	{
		// 아직 플레이어가 안 붙었다. 이 상태로 StartTutorial을 부르면 위젯도 못 만들고
		// InputComponent도 없어서 튜토리얼이 조용히 죽는다 — 준비될 때까지 기다린다.
		startWaitElapsed += startRetryInterval;

		if (startTimeout > 0.f && startWaitElapsed >= startTimeout)
		{
			World->GetTimerManager().ClearTimer(startRetryTimer);
			UE_LOG(LogTemp, Error,
				TEXT("[AC_TutorialGameMode] %.1f초 동안 준비된 AC_PlayerController를 찾지 못해 튜토리얼을 포기합니다."),
				startTimeout);
			return;
		}

		if (!World->GetTimerManager().IsTimerActive(startRetryTimer))
		{
			World->GetTimerManager().SetTimer(
				startRetryTimer, this, &AC_TutorialGameMode::TryStartTutorial, startRetryInterval, true);
		}
		return;
	}

	World->GetTimerManager().ClearTimer(startRetryTimer);

	UC_TutorialComponent* Tutorial = PC->GetTutorial();
	if (!Tutorial)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AC_TutorialGameMode] PlayerController에 UC_TutorialComponent가 없습니다."));
		return;
	}

	bTutorialStarted = true;

	Tutorial->OnTutorialCompleted.AddDynamic(this, &AC_TutorialGameMode::OnTutorialCompleted_Handler);
	Tutorial->StartTutorial(tutorialStepTable, promptWidgetClass);
}

AC_PlayerController* AC_TutorialGameMode::FindReadyPlayerController() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		AC_PlayerController* PC = Cast<AC_PlayerController>(It->Get());
		if (!PC)
		{
			continue;
		}

		// CDO·아키타입이 목록에 섞여 들어오는 경우가 실제로 관측됐다 (레벨 이동 직후).
		// 이런 객체로 CreateWidget을 부르면 "has no Player attached" 에러만 남고 튜토리얼이 죽는다.
		if (PC->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			continue;
		}

		// Player가 붙어야 CreateWidget이 되고, InputComponent가 있어야 입력 감지가 된다
		if (PC->Player == nullptr || PC->InputComponent == nullptr || !PC->IsLocalController())
		{
			continue;
		}

		return PC;
	}

	return nullptr;
}

void AC_TutorialGameMode::OnTutorialCompleted_Handler(int32 CompletedStepCount)
{
	UE_LOG(LogTemp, Log, TEXT("[AC_TutorialGameMode] 튜토리얼 %d단계 완료 — 포탈을 엽니다."), CompletedStepCount);

	ActivateAllPortals();
}
