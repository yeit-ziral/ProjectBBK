// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MonsterSpawnManager.h"
#include "../C_BaseMonster.h"
#include "../M_Gas/C_MonsterASC.h"

void UC_MonsterSpawnManager::Initialize(UWorld* World)
{
    world = World;
}

AActor* UC_MonsterSpawnManager::SpawnMonster(TSubclassOf<AActor> MonsterClass, const FVector& Location, const FRotator& Rotation)
{
    if (!world || !MonsterClass) return nullptr;

    FActorSpawnParameters Params;
    AActor* spawned = world->SpawnActor<AActor>(MonsterClass, Location, Rotation, Params);
    if (!spawned) return nullptr;

    AC_BaseMonster* monster = Cast<AC_BaseMonster>(spawned);
    if (monster && monster->GetMonsterASC())
    {
        aliveCount++;
        monster->GetMonsterASC()->OnMonsterDeath.AddLambda([this](UC_MonsterASC*)
        {
            OnMonsterDied();
        });
    }

    return spawned;
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
