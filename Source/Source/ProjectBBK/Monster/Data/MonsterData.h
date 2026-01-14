#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MonsterData.generated.h"

USTRUCT(BlueprintType)
struct FMonsterData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MonsterId = 0;
    UPROPERTY(EditAnywhere) float MaxHP                         = 100.0f;
    UPROPERTY(EditAnywhere) float MaxGroggy                     = 100.0f;
    UPROPERTY(EditAnywhere) float Attack                        = 20.0f;
    UPROPERTY(EditAnywhere) float Defense                       = 5.0f;
    UPROPERTY(EditAnywhere) float AttackRange                   = 300.0f;
    UPROPERTY(EditAnywhere) float MoveSpeed                     = 600.0f;
    UPROPERTY(EditAnywhere) float NormalCooldown                = 1.5f;
    UPROPERTY(EditAnywhere) float SpecialCooldown               = 6.0f;
};