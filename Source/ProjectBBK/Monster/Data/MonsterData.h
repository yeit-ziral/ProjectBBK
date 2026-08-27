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

    // 스페셜 공격 사거리 — 스페셜 GA 슬롯 1/2에 각각 대응. 0 이하면 AttackRange를 그대로 사용.
    // 원거리 스페셜(엘리트 검은 구체 등)은 노말어택보다 길게 설정할 것.
    UPROPERTY(EditAnywhere, Category = "Attack") float Special1Range = 0.0f;
    UPROPERTY(EditAnywhere, Category = "Attack") float Special2Range = 0.0f;

    // 스페셜 슬롯별 개별 쿨다운(초). 0 이하면 SpecialCooldown을 공유한다.
    // 값을 넣으면 그 슬롯만 독립 쿨다운으로 돈다 (엘리트 수렁 장판 등).
    UPROPERTY(EditAnywhere, Category = "Attack") float Special1Cooldown = 0.0f;
    UPROPERTY(EditAnywhere, Category = "Attack") float Special2Cooldown = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Reposition") bool  bEnableReposition      = true;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionDesiredRange = 250.0f;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionMinRange     = 0.0f;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionSpeed        = 300.0f;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionStrafeWeight = 0.5f;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionBand         = 60.0f;
    UPROPERTY(EditAnywhere, Category = "Reposition") float RepositionFlipInterval = 2.5f;

    UPROPERTY(EditAnywhere, Category = "Reward") float ExpReward = 0.0f;
};