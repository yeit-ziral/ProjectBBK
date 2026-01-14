// Fill out your copyright notice in the Description page of Project Settings.


#include "C_GameModeTest.h"

AC_GameModeTest::AC_GameModeTest()
{

}

void AC_GameModeTest::BeginPlay()
{
	Super::BeginPlay();

    monsterSpawnManager = NewObject<UC_MonsterSpawnManager>(this);
    monsterSpawnManager->Initialize(GetWorld());

    UE_LOG(LogTemp, Warning, TEXT("SpawnManager created: %s"), monsterSpawnManager ? TEXT("OK") : TEXT("FAILED"));

    if (monsterSpawnManager && *testMonsterClass)
    {
        const FVector SpawnLocation(0.f, 0.f, 200.f);
        const FRotator SpawnRotation = FRotator::ZeroRotator;

        AActor* Spawned = monsterSpawnManager->SpawnMonster(testMonsterClass, SpawnLocation, SpawnRotation);
        UE_LOG(LogTemp, Warning, TEXT("Test Spawn Result: %s"), Spawned ? *Spawned->GetName() : TEXT("Failed"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TestMonsterClass is not set."));
    }
}
