// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_EliteSummonCircle.generated.h"

class AC_BaseMonster;
class UDecalComponent;
class UBillboardComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEliteSummoned, AC_BaseMonster*, SummonedMonster);

// 레벨 몬스터 전멸 시 소환 마법진을 바닥에 띄우고, 그 아래에서 엘리트 몬스터가 천천히 솟아오르게 한다.
// 솟아오르는 동안 몬스터는 콜리전·AI·Tick이 꺼져 있고(플레이어와 상호작용 없음), 다 올라온 순간 한꺼번에 켠다.
// 마법진은 보스 등장 마법진과 같은 무늬를 쓰는 전용 머티리얼(M_EliteSummonCircle, 초록 발광).
// AC_BBKGameMode::OnAllMonstersDefeated를 받아 RegisterPendingMonster()로 엘리트를 미리 집계에 넣으므로
// 포탈은 엘리트까지 죽어야 열린다. 레벨에 배치만 하면 동작 (배치 위치 = 소환 위치, 높이는 바닥 트레이스로 보정).
UCLASS()
class PROJECTBBK_API AC_EliteSummonCircle : public AActor
{
	GENERATED_BODY()

public:
	AC_EliteSummonCircle();

	virtual void Tick(float DeltaSeconds) override;

	// 엘리트가 다 올라와 활성화된 순간 1회 브로드캐스트 (연출 연결용)
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

	// 마법진이 나타난 뒤 몬스터가 솟아오르기 시작할 때까지 대기 시간
	UPROPERTY(EditAnywhere, Category = "Summon", meta = (ClampMin = "0"))
	float summonDelay = 1.5f;

	// 몬스터가 바닥 아래에서 완전히 올라오는 데 걸리는 시간
	UPROPERTY(EditAnywhere, Category = "Summon", meta = (ClampMin = "0.1"))
	float riseDuration = 3.5f;

	// 시작 시 몬스터 키(캡슐 높이)보다 더 깊이 묻는 여유 (cm) — 머리 장식·무기가 바닥 위로 삐져나오지 않게
	UPROPERTY(EditAnywhere, Category = "Summon", meta = (ClampMin = "0"))
	float riseExtraDepth = 80.f;

	// 다 올라온 뒤 마법진이 남아 있는 시간 (이후 circleFadeTime 동안 사라짐)
	UPROPERTY(EditAnywhere, Category = "Summon", meta = (ClampMin = "0"))
	float circleLingerTime = 1.0f;

	// 마법진이 서서히 나타나고 사라지는 시간
	UPROPERTY(EditAnywhere, Category = "Summon|Visual", meta = (ClampMin = "0"))
	float circleFadeTime = 0.6f;

	// 기본값 M_EliteSummonCircle. (이전 이름 circleMaterial에서 바꿈 — 레벨 인스턴스에 저장된 예전 값이 따라오지 않게)
	UPROPERTY(EditAnywhere, Category = "Summon|Visual")
	TObjectPtr<UMaterialInterface> summonCircleMaterial;

	// 머티리얼 파라미터 이름 — 해당 파라미터가 없으면 무시된다
	UPROPERTY(EditAnywhere, Category = "Summon|Visual")
	FName colorParameterName = TEXT("Color");

	UPROPERTY(EditAnywhere, Category = "Summon|Visual")
	FName intensityParameterName = TEXT("Intensity");

	UPROPERTY(EditAnywhere, Category = "Summon|Visual")
	FName opacityParameterName = TEXT("Opacity");

	// (이전 이름 circleColor에서 바꿈 — 같은 이유)
	UPROPERTY(EditAnywhere, Category = "Summon|Visual")
	FLinearColor summonCircleColor = FLinearColor(0.1f, 1.0f, 0.25f, 1.0f);

	// 발광 세기
	UPROPERTY(EditAnywhere, Category = "Summon|Visual", meta = (ClampMin = "0"))
	float circleIntensity = 3.f;

	// 바닥 투영 반경(Y·Z)과 투영 깊이(X)
	UPROPERTY(EditAnywhere, Category = "Summon|Visual", meta = (ClampMin = "1"))
	float circleRadius = 400.f;

	UPROPERTY(EditAnywhere, Category = "Summon|Visual", meta = (ClampMin = "1"))
	float decalDepth = 200.f;

private:
	void HandleAllMonstersDefeated();

	// 몬스터를 스폰해 바닥 아래에 묻고(비활성 상태) 솟아오르기 시작
	void BeginRise();

	// 다 올라옴 — 콜리전·이동·Tick·AI를 켜서 전투 가능 상태로
	void FinishRise();

	void StartCircleFadeOut();
	void SetCircleOpacity(float Opacity);

	// 액터 위치에서 아래로 트레이스해 바닥 좌표를 구함 (실패 시 액터 위치 그대로)
	FVector FindGroundLocation() const;

	bool bTriggered = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> circleMID;

	// 솟아오르는 중인 몬스터와 그 도착 위치
	TWeakObjectPtr<AC_BaseMonster> risingMonster;
	FVector riseTargetLocation = FVector::ZeroVector;
	float riseDepth = 0.f;
	float riseElapsed = 0.f;
	bool bRising = false;

	// 마법진 페이드: +1 = 나타나는 중, -1 = 사라지는 중, 0 = 정지
	int32 circleFadeDirection = 0;
	float circleOpacity = 0.f;

	FTimerHandle summonTimer;
	FTimerHandle hideTimer;
};
