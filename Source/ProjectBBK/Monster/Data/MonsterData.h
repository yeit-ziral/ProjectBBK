#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MonsterData.generated.h"

USTRUCT(BlueprintType)
struct FMonsterData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Id = 1001;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 HP = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Attack = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MoveSpeed = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackRange = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackCoolDown = 1.0f;
};