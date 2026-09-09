// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "C_BBKGameMode.generated.h"

class AC_Portal;

UCLASS()
class PROJECTBBK_API AC_BBKGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AC_BBKGameMode();

	// C_BaseMonster::ExecuteDeathSequence()에서 호출 — 남은 몬스터 수 감소 및 포탈 활성화 체크
	void NotifyMonsterDead();

protected:
	virtual void BeginPlay() override;

	// 몬스터 전멸만으로 포탈을 열어도 되는지. 튜토리얼처럼 별도 완료 조건이 있는 레벨은 false를 반환하고
	// 자체 타이밍에 ActivateAllPortals()를 직접 호출한다.
	virtual bool ShouldAutoActivatePortals() const { return true; }

	void ActivateAllPortals();

private:
	UFUNCTION()
	void OnPortalEntered_Handler(AC_Portal* Portal);

	TArray<TWeakObjectPtr<AC_Portal>> RegisteredPortals;
	int32 RemainingMonsterCount = 0;
};
