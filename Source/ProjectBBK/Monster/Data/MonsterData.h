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

    UPROPERTY(EditAnywhere, Category = "Reposition") bool  bEnableReposition      = true;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionDesiredRange = 250.0f;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionMinRange     = 0.0f;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionSpeed        = 300.0f;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionStrafeWeight = 0.5f;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionBand         = 60.0f;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionFlipInterval = 2.5f;
};