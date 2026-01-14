// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
//#include "../Data/MonsterSpawnData.h"
#include "C_MonsterSpawnManager.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_MonsterSpawnManager : public UObject
{
	GENERATED_BODY()
public:
    void Initialize(UWorld* World);

    class AActor* SpawnMonster(TSubclassOf<AActor> MonsterClass, const FVector& Location, const FRotator& Rotation);


    void SetSpawnDataTable(UDataTable* InTable);

private:
    UPROPERTY()
    UWorld* world;
	
};
