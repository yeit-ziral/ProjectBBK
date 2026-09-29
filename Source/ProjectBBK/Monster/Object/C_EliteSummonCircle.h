// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_EliteSummonCircle.generated.h"

class AC_BaseMonster;
class UDecalComponent;
class UBillboardComponent;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEliteSummoned, AC_BaseMonster*, SummonedMonster);

// 레벨 몬스터 전멸 시 소환 마법진을 바닥에 띄우고, 잠시 후 그 자리에서 엘리트 몬스터를 스폰한다.
// 보스 등장 마법진(MagicCircleFiredecal)을 다이나믹 머티리얼로 색만 바꿔 재사용.
// AC_BBKGameMode::OnAllMonstersDefeated를 받아 RegisterPendingMonster()로 엘리트를 미리 집계에 넣으므로
// 포탈은 엘리트까지 죽어야 열린다. 레벨에 배치만 하면 동작 (배치 위치 = 소환 위치, 높이는 바닥 트레이스로 보정).
UCLASS()
class PROJECTBBK_API AC_EliteSummonCircle : public AActor
{
	GENERATED_BODY()

public:
	AC_EliteSummonCircle();

	// 엘리트 스폰 직후 1회 브로드캐스트 (연출 연결용)
	UPROPERTY(BlueprintAssignable, Category = "Summon")
	FOnEliteSummoned OnEliteSummoned;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Summon")
	TObjectPtr<USceneComponent> root;

	// 바닥 투영 마법진 — 전멸 전까지 숨김
	UPROPERTY(VisibleAnywhere, Category = "Summon")
	TObjectPtr<UDecalComponent> circleDecal;

	UPROPERTY(VisibleAnywhere, Category = "Summon")
	TObjectPtr<UBillboardComponent> editorIcon;

	// 소환할 몬스터 — 기본값 BPC_EliteMonster
	UPROPERTY(EditAnywhere, Category = "Summon")
	TSubclassOf<AC_BaseMonster> summonClass;

	UPROPERTY(EditAnywhere, Category = "Summon", meta = (ClampMin = "1"))
	int32 summonLevel = 7;

	// 마법진이 나타난 뒤 몬스터가 스폰되기까지 대기 시간
	UPROPERTY(EditAnywhere, Category = "Summon", meta = (ClampMin = "0"))
	float summonDelay = 2.0f;

	// 스폰 후 마법진이 남아 있는 시간 (0이면 스폰과 동시에 사라짐)
	UPROPERTY(EditAnywhere, Category = "Summon", meta = (ClampMin = "0"))
	float circleLingerTime = 2.0f;

	// 기본값 MagicCircleFiredecal (보스 등장 마법진)
	UPROPERTY(EditAnywhere, Category = "Summon|Visual")
	TObjectPtr<UMaterialInterface> circleMaterial;

	// 머티리얼의 색 벡터 파라미터 이름 — 해당 파라미터가 없으면 원본 색 그대로
	UPROPERTY(EditAnywhere, Category = "Summon|Visual")
	FName colorParameterName = TEXT("Color");

	UPROPERTY(EditAnywhere, Category = "Summon|Visual")
	FLinearColor circleColor = FLinearColor(0.55f, 0.1f, 1.0f, 1.0f);

	// 바닥 투영 반경(Y·Z)과 투영 깊이(X)
	UPROPERTY(EditAnywhere, Category = "Summon|Visual", meta = (ClampMin = "1"))
	float circleRadius = 400.f;

	UPROPERTY(EditAnywhere, Category = "Summon|Visual", meta = (ClampMin = "1"))
	float decalDepth = 200.f;

private:
	void HandleAllMonstersDefeated();
	void SpawnSummon();
	void HideCircle();

	// 액터 위치에서 아래로 트레이스해 바닥 좌표를 구함 (실패 시 액터 위치 그대로)
	FVector FindGroundLocation() const;

	bool bTriggered = false;

	FTimerHandle summonTimer;
	FTimerHandle hideTimer;
};
