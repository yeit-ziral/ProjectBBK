// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Manager/C_MonsterSpawnManager.h"

#include "C_GameModeTest.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API AC_GameModeTest : public AGameModeBase
{
	GENERATED_BODY()
public:
	AC_GameModeTest();

	class UC_MonsterSpawnManager* GetMonsterSpawnManager() const { return monsterSpawnManager; }


protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UC_MonsterSpawnManager* monsterSpawnManager;

	UPROPERTY(EditDefaultsOnly, Category = "Monster|Test")
	TSubclassOf<AActor> testMonsterClass;
};
