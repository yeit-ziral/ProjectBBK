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

	UPROPERTY()
	AC_BaseMonster* ownerMonster = nullptr;
};
