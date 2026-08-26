// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "C_MonsterDataComponent.generated.h"

class AC_BaseMonster;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTBBK_API UC_MonsterDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UC_MonsterDataComponent();

	// HasAuthority() 확인 후 BeginPlay에서 호출
	void Initialize(AC_BaseMonster* InOwner);

	FName GetRowName() const { return rowName; }
	int32 GetMonsterId() const { return monsterId; }

	bool  IsRepositionEnabled()       const { return bEnableReposition; }
	float GetRepositionDesiredRange() const { return repositionDesiredRange; }
	float GetRepositionMinRange()     const { return repositionMinRange; }
	float GetRepositionSpeed()        const { return repositionSpeed; }
	float GetRepositionStrafeWeight() const { return repositionStrafeWeight; }
	float GetRepositionBand()         const { return repositionBand; }
	float GetRepositionFlipInterval() const { return repositionFlipInterval; }
	float GetExpReward()              const { return expReward; }

	// 스페셜 공격 사거리 — DT에서 0 이하면 Initialize에서 AttackRange로 해석해 둔 값
	float GetSpecial1Range()          const { return special1Range; }
	float GetSpecial2Range()          const { return special2Range; }

	// 스페셜 슬롯별 쿨다운 — DT에서 0 이하면 Initialize에서 SpecialCooldown으로 해석해 둔 값
	float GetSpecial1Cooldown()       const { return special1Cooldown; }
	float GetSpecial2Cooldown()       const { return special2Cooldown; }

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", meta = (AllowPrivateAccess = "true"))
	FName rowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UDataTable> monsterTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	int32 monsterId = 0;

	bool  bEnableReposition      = true;
	float repositionDesiredRange = 250.f;
	float repositionMinRange     = 0.f;
	float repositionSpeed        = 300.f;
	float repositionStrafeWeight = 0.5f;
	float repositionBand         = 60.f;
	float repositionFlipInterval = 2.5f;
	float expReward              = 0.f;
	float special1Range          = 0.f;
	float special2Range          = 0.f;
	float special1Cooldown       = 0.f;
	float special2Cooldown       = 0.f;

	UPROPERTY()
	AC_BaseMonster* ownerMonster = nullptr;
};
