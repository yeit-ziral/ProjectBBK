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
#include "TimerManager.h"
#include "../C_BossMonster.h"

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

	// 레벨 시작 시점부터 보스를 숨기고 AI를 정지 — 컷신 전 플레이어 인식·패턴 사용 차단
	if (bossActor)
	{
		bossActor->SetActorHiddenInGame(true);
		bossActor->SetActorEnableCollision(false);
		SetBossAIActive(false);

		// AIController 빙의가 트리거 BeginPlay 이후일 수 있으므로 다음 틱에 한 번 더 정지
		GetWorldTimerManager().SetTimerForNextTick(this, &AC_BossCutsceneTrigger::StopBossLogicDeferred);

		// 보스 HP UI도 컷신 종료 전까지 숨김
		if (AC_BossMonster* boss = Cast<AC_BossMonster>(bossActor))
		{
			boss->SetHpWidgetSuppressed(true);
		}
	}

	// 마법진도 보스 등장 전까지 숨김 — RevealBoss에서 함께 나타남
	if (magicCircleActor)
	{
		magicCircleActor->SetActorHiddenInGame(true);
	}

	// 플레이어가 트리거 안에 이미 겹쳐진 채 스폰된 경우 BeginOverlap이 발동 안 함 →
	// 다음 틱(플레이어 스폰·오버랩 등록 이후)에 직접 검사
	GetWorldTimerManager().SetTimerForNextTick(this, &AC_BossCutsceneTrigger::CheckInitialOverlap);
}

void AC_BossCutsceneTrigger::CheckInitialOverlap()
{
	if (bCutsceneStarted) return;

	TArray<AActor*> overlappingActors;
	triggerBox->GetOverlappingActors(overlappingActors, ACharacter::StaticClass());

	for (AActor* actor : overlappingActors)
	{
		ACharacter* playerChar = Cast<ACharacter>(actor);
		if (playerChar && Cast<APlayerController>(playerChar->GetController()))
		{
			BeginCutscene(playerChar);
			return;
		}
	}
}

void AC_BossCutsceneTrigger::OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	BeginCutscene(Cast<ACharacter>(OtherActor));
}

void AC_BossCutsceneTrigger::BeginCutscene(ACharacter* playerChar)
{
	if (bCutsceneStarted) return;
	if (!cutsceneSequence) return;
	if (!playerChar) return;

	APlayerController* pc = Cast<APlayerController>(playerChar->GetController());
	if (!pc) return;

	bCutsceneStarted = true;

	// 트리거 비활성화 (중복 재생 방지)
	triggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 입력 차단
	pc->DisableInput(pc);
	pc->SetShowMouseCursor(false);

	// 실제 플레이어 숨김 (컷신에는 시퀀서의 Spawnable 플레이어가 출연)
	hiddenPlayer = playerChar;
	playerChar->SetActorHiddenInGame(true);

	// 보스는 계속 숨김·AI 정지 상태 유지 — 컷신 중 '소환' 프레임에 RevealBoss로 드러냄

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

void AC_BossCutsceneTrigger::RevealBoss()
{
	if (bBossRevealed) return;
	if (!bossActor) return;

	bBossRevealed = true;

	// 숨김만 해제 — 보스는 아직 땅 밑이라 보이지 않다가 시퀀스가 끌어올리며 솟아오름.
	// 콜리전은 컷신 종료 시 켬 (올라오는 동안 무브먼트가 시퀀스 위치 제어와 충돌하지 않도록)
	bossActor->SetActorHiddenInGame(false);

	// 마법진 등장 — 보스 솟아오름과 함께
	if (magicCircleActor)
	{
		magicCircleActor->SetActorHiddenInGame(false);
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

	// 보스 표시·콜리전·AI, 마법진 제거는 모두 다음 틱(시퀀서 종료 처리가 끝난 후)에 확정 적용.
	// OnFinished 시점은 시퀀서가 아직 바인딩/상태 복원을 처리 중이라, 여기서 시퀀스가
	// 참조하는 액터를 Destroy하면 댕글링 참조로 크래시 발생
	GetWorldTimerManager().SetTimerForNextTick(this, &AC_BossCutsceneTrigger::FinalizeCombatStart);
}

void AC_BossCutsceneTrigger::FinalizeCombatStart()
{
	// 마법진 제거 — 컷신 종료와 함께 사라짐 (시퀀서가 완전히 끝난 다음 틱이라 안전)
	if (magicCircleActor)
	{
		magicCircleActor->Destroy();
	}

	// 시퀀서 복원으로 다시 숨겨졌어도 여기서 확실히 표시 (복원은 이미 끝난 시점)
	if (bossActor)
	{
		bossActor->SetActorHiddenInGame(false);
		bossActor->SetActorEnableCollision(true);
	}

	// 보스 HP UI 표시 — 컷신 종료, 전투 시작
	if (AC_BossMonster* boss = Cast<AC_BossMonster>(bossActor))
	{
		boss->SetHpWidgetSuppressed(false);
	}

	// 보스 AI 재개 → 전투 시작
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

void AC_BossCutsceneTrigger::StopBossLogicDeferred()
{
	SetBossAIActive(false);
}
