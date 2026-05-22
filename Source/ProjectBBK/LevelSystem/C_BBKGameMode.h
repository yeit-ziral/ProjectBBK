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

private:
	UFUNCTION()
	void OnPortalEntered_Handler(AC_Portal* Portal);

	void ActivateAllPortals();

	TArray<TWeakObjectPtr<AC_Portal>> RegisteredPortals;
	int32 RemainingMonsterCount = 0;
};
