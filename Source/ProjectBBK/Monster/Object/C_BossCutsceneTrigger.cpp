// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BossCutsceneTrigger.h"
#include "Components/BoxComponent.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "BrainComponent.h"

AC_BossCutsceneTrigger::AC_BossCutsceneTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	triggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(triggerBox);
	triggerBox->SetBoxExtent(FVector(200.f, 200.f, 100.f));
	triggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void AC_BossCutsceneTrigger::BeginPlay()
{
	Super::BeginPlay();

	triggerBox->OnComponentBeginOverlap.AddDynamic(this, &AC_BossCutsceneTrigger::OnTriggerOverlap);
}

void AC_BossCutsceneTrigger::OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!cutsceneSequence) return;

	ACharacter* playerChar = Cast<ACharacter>(OtherActor);
	if (!playerChar) return;

	APlayerController* pc = Cast<APlayerController>(playerChar->GetController());
	if (!pc) return;

	// 트리거 비활성화 (중복 재생 방지)
	triggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 입력 차단
	pc->DisableInput(pc);
	pc->SetShowMouseCursor(false);

	// 실제 플레이어 숨김 (컷신에는 시퀀서의 Spawnable 플레이어가 출연)
	hiddenPlayer = playerChar;
	playerChar->SetActorHiddenInGame(true);

	// 보스 AI 정지 — 컷신 중 패턴 사용 방지
	SetBossAIActive(false);

	// 시퀀스 재생
	FMovieSceneSequencePlaybackSettings settings;
	settings.bAutoPlay = false;

	sequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(), cutsceneSequence, settings, sequenceActor);

	if (sequencePlayer)
	{
		sequencePlayer->OnFinished.AddDynamic(this, &AC_BossCutsceneTrigger::OnCutsceneFinished);
		sequencePlayer->Play();
	}
}

void AC_BossCutsceneTrigger::OnCutsceneFinished()
{
	APlayerController* pc = GetWorld()->GetFirstPlayerController();
	if (pc)
	{
		pc->EnableInput(pc);
	}

	// 플레이어 다시 표시
	if (hiddenPlayer)
	{
		hiddenPlayer->SetActorHiddenInGame(false);
	}

	// 보스 AI 재개 — 이제부터 전투 시작
	SetBossAIActive(true);

	Destroy();
}

void AC_BossCutsceneTrigger::SetBossAIActive(bool bActive)
{
	APawn* bossPawn = Cast<APawn>(bossActor);
	if (!bossPawn) return;

	AAIController* aic = Cast<AAIController>(bossPawn->GetController());
	if (!aic) return;

	UBrainComponent* brain = aic->GetBrainComponent();
	if (!brain) return;

	if (bActive)
		brain->RestartLogic();
	else
		brain->StopLogic(TEXT("BossCutscene"));
}
