#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MonsterSpawnData.generated.h"

USTRUCT(BlueprintType)
struct FMonsterSpawnRow : public FTableRowBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MonsterId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<AActor> MonsterClass;
};