// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MonsterSpawnManager.h"
#include "../C_BaseMonster.h"
#include "../M_Gas/C_MonsterASC.h"

void UC_MonsterSpawnManager::Initialize(UWorld* World)
{
    world = World;
}

AActor* UC_MonsterSpawnManager::SpawnMonster(TSubclassOf<AActor> MonsterClass, const FVector& Location, const FRotator& Rotation, int32 InLevel)
{
    if (!world || !MonsterClass) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // BeginPlay 전에 level을 설정하기 위해 deferred spawn 사용
    AC_BaseMonster* monster = world->SpawnActorDeferred<AC_BaseMonster>(MonsterClass, FTransform(Rotation, Location), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (!monster) return nullptr;

    monster->level = InLevel;

    monster->FinishSpawning(FTransform(Rotation, Location));

    if (monster->GetMonsterASC())
    {
        aliveCount++;
        monster->GetMonsterASC()->OnMonsterDeath.AddLambda([this](UC_MonsterASC*)
        {
            OnMonsterDied();
        });
    }

    return monster;
}

void UC_MonsterSpawnManager::Reset()
{
    aliveCount = 0;
}

void UC_MonsterSpawnManager::OnMonsterDied()
{
    aliveCount = FMath::Max(0, aliveCount - 1);

    if (aliveCount == 0)
        OnAllMonstersDefeated.Broadcast();
}

void UC_MonsterSpawnManager::SetSpawnDataTable(UDataTable* InTable)
{
}
