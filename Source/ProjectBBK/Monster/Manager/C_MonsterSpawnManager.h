// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "C_MonsterSpawnManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllMonstersDefeated);

/**
 *
 */
UCLASS(BlueprintType)
class PROJECTBBK_API UC_MonsterSpawnManager : public UObject
{
	GENERATED_BODY()

public:
    void Initialize(UWorld* World);

    // 몬스터 스폰 + 생존 추적 자동 등록 (InLevel: 레벨에 따라 MaxHP = BaseHP + Level * 50)
    UFUNCTION(BlueprintCallable, Category = "Spawn")
    AActor* SpawnMonster(TSubclassOf<AActor> MonsterClass, const FVector& Location, const FRotator& Rotation, int32 InLevel = 1);

    // 새 던전 시작 시 호출 — 카운터 초기화
    UFUNCTION(BlueprintCallable, Category = "Spawn")
    void Reset();

    UFUNCTION(BlueprintPure, Category = "Spawn")
    int32 GetAliveCount() const { return aliveCount; }

    // 모든 몬스터 사망 시 브로드캐스트
    UPROPERTY(BlueprintAssignable, Category = "Spawn")
    FOnAllMonstersDefeated OnAllMonstersDefeated;

    void SetSpawnDataTable(UDataTable* InTable);

private:
    UPROPERTY()
    UWorld* world;

    int32 aliveCount = 0;

    void OnMonsterDied();
};
