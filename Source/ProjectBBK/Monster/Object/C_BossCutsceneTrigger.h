// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_BossCutsceneTrigger.generated.h"

class UBoxComponent;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;

UCLASS()
class PROJECTBBK_API AC_BossCutsceneTrigger : public AActor
{
	GENERATED_BODY()

public:
	AC_BossCutsceneTrigger();

	// 시퀀서 Event Track에서 호출 — 컷신 중간 '보스 소환' 시점에 보스를 화면에 드러냄.
	// 중복 호출돼도 1회만 적용됨 (OnCutsceneFinished 안전망과 공유)
	UFUNCTION(BlueprintCallable, Category = "Cutscene")
	void RevealBoss();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Trigger")
	UBoxComponent* triggerBox;

	// 에디터에서 Boss 시퀀스 할당
	UPROPERTY(EditAnywhere, Category = "Cutscene")
	ULevelSequence* cutsceneSequence;

	// 레벨에 배치된 보스 액터 (시퀀스가 애니메이션을 재생하는 그 보스)
	UPROPERTY(EditAnywhere, Category = "Cutscene")
	AActor* bossActor;

	// 레벨에 배치된 소환 마법진 — 보스 등장 시 함께 나타나고 컷신 종료 시 사라짐
	UPROPERTY(EditAnywhere, Category = "Cutscene")
	AActor* magicCircleActor;

private:
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 컷신 시작 로직 (오버랩·초기 검사 공통 진입점)
	void BeginCutscene(ACharacter* playerChar);

	// 플레이어가 트리거 안에 이미 겹쳐진 채 스폰된 경우 대비 — BeginPlay 다음 틱 검사
	void CheckInitialOverlap();

	UFUNCTION()
	void OnCutsceneFinished();

	// 보스 AI 정지/재개
	void SetBossAIActive(bool bActive);

	// 빙의 타이밍 대비 — 다음 틱에 AI 정지를 한 번 더 보장
	void StopBossLogicDeferred();

	// 컷신 종료 다음 틱 — 시퀀서 상태 복원 이후 보스 표시·콜리전·AI 확정
	void FinalizeCombatStart();

	UPROPERTY()
	ULevelSequencePlayer* sequencePlayer;

	UPROPERTY()
	ALevelSequenceActor* sequenceActor;

	// 컷신 동안 숨겨둔 실제 플레이어
	UPROPERTY()
	ACharacter* hiddenPlayer;

	// 보스 중복 노출 방지 (Event Track + OnCutsceneFinished 안전망 공유)
	bool bBossRevealed = false;

	// 컷신 중복 시작 방지 (오버랩 + 초기 검사 공유)
	bool bCutsceneStarted = false;
};
