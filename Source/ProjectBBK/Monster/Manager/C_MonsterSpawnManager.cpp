// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterSpawnManager.h"

void UC_MonsterSpawnManager::Initialize(UWorld* World)
{
    world = World;
}

AActor* UC_MonsterSpawnManager::SpawnMonster(TSubclassOf<AActor> MonsterClass, const FVector& Location, const FRotator& Rotation)
{
    if (!world || !*MonsterClass)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    return world->SpawnActor<AActor>(MonsterClass, Location, Rotation, Params);
}

void UC_MonsterSpawnManager::SetSpawnDataTable(UDataTable* InTable)
{
}
