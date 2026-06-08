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

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Trigger")
	UBoxComponent* triggerBox;

	// 에디터에서 Boss 시퀀스 할당
	UPROPERTY(EditAnywhere, Category = "Cutscene")
	ULevelSequence* cutsceneSequence;

	// 컷신 동안 AI를 멈출 보스 액터 (레벨에서 할당)
	UPROPERTY(EditAnywhere, Category = "Cutscene")
	AActor* bossActor;

private:
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnCutsceneFinished();

	// 보스 AI 정지/재개
	void SetBossAIActive(bool bActive);

	UPROPERTY()
	ULevelSequencePlayer* sequencePlayer;

	UPROPERTY()
	ALevelSequenceActor* sequenceActor;

	// 컷신 동안 숨겨둔 실제 플레이어
	UPROPERTY()
	ACharacter* hiddenPlayer;
};
