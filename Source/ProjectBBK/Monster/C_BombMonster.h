// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_BaseMonster.h"
#include "C_BombMonster.generated.h"

class UGameplayEffect;
class UNiagaraSystem;
class USoundBase;

/**
 * 자폭 몬스터 — 공격 GA를 가지지 않는 비행 몬스터.
 * 플레이어를 인식하면 비행으로 돌진하고, 접촉하면 퓨즈(fuseTime) 후 폭발한다.
 * 폭발 데미지는 "대상 최대 체력의 explosionHealthPercent 비율" 고정값
 * (AC_BossDangerZone과 동일한 방식 — 몬스터 Attack 스탯 미사용).
 *
 * BT 없이 Tick으로 타겟 탐색·추격을 처리하므로 behaviorTree 미할당 상태로 동작한다.
 * (BT를 붙이려면 Tick 추격 대신 별도 Task를 만들어야 함)
 */
UCLASS()
class PROJECTBBK_API AC_BombMonster : public AC_BaseMonster
{
	GENERATED_BODY()

public:
	AC_BombMonster();

	// 자폭 몬스터는 공격 GA가 없음 — BT 공격 브랜치가 붙어도 항상 실패
	virtual bool CanAutoAttack() const override { return false; }

	// 기폭 시작 (퓨즈 → 폭발). 외부 트리거에서도 호출 가능
	UFUNCTION(BlueprintCallable, Category = "Bomb")
	void StartFuse();

	// 즉시 폭발 — 데미지 적용 + VFX/사운드 + 자신 사망 처리
	UFUNCTION(BlueprintCallable, Category = "Bomb")
	void Explode();

	UFUNCTION(BlueprintPure, Category = "Bomb")
	bool HasExploded() const { return bExploded; }

	UFUNCTION(BlueprintPure, Category = "Bomb")
	bool IsFuseStarted() const { return bFuseStarted; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void ExecuteDeathSequence() override;

	// 그로기 해제·피격 반응 종료 시 외부에서 MOVE_Walking으로 되돌려 추락하는 것을 방지
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

#pragma region Detection
	// 플레이어 인식 거리
	UPROPERTY(EditAnywhere, Category = "Bomb|Detection")
	float detectionRange = 1500.f;

	// 타겟 재탐색 주기(초)
	UPROPERTY(EditAnywhere, Category = "Bomb|Detection")
	float targetSearchInterval = 0.25f;
#pragma endregion

#pragma region Movement
	// 타겟 위치에서 이만큼 위를 조준해 비행 (지면에 박히지 않게 함)
	UPROPERTY(EditAnywhere, Category = "Bomb|Movement")
	float hoverHeightOffset = 40.f;

	// 비행 가속도 — 클수록 방향 전환이 민첩
	UPROPERTY(EditAnywhere, Category = "Bomb|Movement")
	float flyAcceleration = 2048.f;
#pragma endregion

#pragma region Weave (돌진 중 S자 곡선)
	// 좌우 흔들림 세기 — 진행 방향 대비 횡방향 블렌드 비율. 0이면 직진
	UPROPERTY(EditAnywhere, Category = "Bomb|Weave", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float weaveAmplitude = 0.7f;

	// 좌우 흔들림 주기(Hz) — 초당 S자 왕복 횟수.
	// 값이 클수록 폭이 좁아져 S자가 아닌 잔떨림이 된다 (PIE 측정: 1.2Hz는 좌우 편차 약 29cm,
	// 0.35Hz는 47cm이며 반대편까지 넘어감 — 속도 150 기준). 넓은 S자를 원하면 0.3~0.5 권장
	UPROPERTY(EditAnywhere, Category = "Bomb|Weave", meta = (ClampMin = "0.0"))
	float weaveFrequency = 0.35f;

	// 상하 흔들림 세기 — 좌우보다 빠른 주기로 흔들려 큰 S자 위에 잔진동이 얹힌다
	UPROPERTY(EditAnywhere, Category = "Bomb|Weave", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float weaveVerticalAmplitude = 0.25f;

	// 상하 흔들림 주기 = 좌우 주기 × 이 값. 정수배를 피해야 궤적이 같은 모양으로 반복되지 않음
	UPROPERTY(EditAnywhere, Category = "Bomb|Weave", meta = (ClampMin = "0.0"))
	float weaveVerticalFrequencyRatio = 2.7f;

	// 타겟까지 이 거리 이내로 들어오면 흔들림을 점차 줄여 직선으로 파고듦
	// (끝까지 흔들면 플레이어 주위를 맴돌기만 하고 접촉하지 못함)
	UPROPERTY(EditAnywhere, Category = "Bomb|Weave", meta = (ClampMin = "0.0"))
	float weaveFalloffDistance = 350.f;
#pragma endregion

#pragma region Patrol (인식 전 배회)
	// BeginPlay 위치(홈) 기준 배회 반경
	UPROPERTY(EditAnywhere, Category = "Bomb|Patrol")
	float patrolRadius = 400.f;

	// 배회 비행 속도 — 돌진 속도(moveSpeed)보다 느리게 두는 것을 권장
	UPROPERTY(EditAnywhere, Category = "Bomb|Patrol")
	float patrolSpeed = 250.f;

	// 배회 목표점의 홈 대비 상하 편차 범위
	UPROPERTY(EditAnywhere, Category = "Bomb|Patrol")
	float patrolVerticalRange = 120.f;

	// 목표점에 이만큼 접근하면 도달로 간주하고 다음 지점 선정
	UPROPERTY(EditAnywhere, Category = "Bomb|Patrol")
	float patrolPointTolerance = 120.f;

	// 목표점에 도달하지 못해도 이 시간이 지나면 재선정 (벽에 낀 경우 대비)
	UPROPERTY(EditAnywhere, Category = "Bomb|Patrol")
	float patrolRepickInterval = 4.f;

	// 배회 중 제자리 부유감 — 상하 진폭(cm)과 주기(Hz)
	UPROPERTY(EditAnywhere, Category = "Bomb|Patrol")
	float patrolBobAmplitude = 25.f;

	UPROPERTY(EditAnywhere, Category = "Bomb|Patrol")
	float patrolBobFrequency = 0.6f;
#pragma endregion

#pragma region Explosion
	// 이 거리 안으로 들어오면 접촉으로 판정해 기폭 (캡슐 Hit 이벤트와 병행)
	UPROPERTY(EditAnywhere, Category = "Bomb|Explosion")
	float contactDistance = 120.f;

	// 접촉(기폭) ~ 폭발까지 지연(초). 이 동안 몬스터는 정지하고 점멸하므로
	// 플레이어가 폭발 반경 밖으로 벗어날 시간이 된다. 0이면 접촉 즉시 폭발
	UPROPERTY(EditAnywhere, Category = "Bomb|Explosion")
	float fuseTime = 3.0f;

	// 폭발 피해 반경
	UPROPERTY(EditAnywhere, Category = "Bomb|Explosion")
	float explosionRadius = 300.f;

	// 대상 최대 체력 대비 폭발 데미지 비율 (0.3 = 30%)
	UPROPERTY(EditAnywhere, Category = "Bomb|Explosion")
	float explosionHealthPercent = 0.3f;

	// true면 피격으로 사망할 때도 폭발
	UPROPERTY(EditAnywhere, Category = "Bomb|Explosion")
	bool bExplodeOnDeath = false;

	// 폭발 데미지 GE — BP에서 할당 (방어력 무시 고정 데미지를 원하면 GE_DotDamage 계열)
	UPROPERTY(EditDefaultsOnly, Category = "Bomb|Explosion")
	TSubclassOf<UGameplayEffect> explosionDamageGEClass;

	// Set by Caller 태그 — 미설정 시 BeginPlay에서 Data.Damage로 폴백
	UPROPERTY(EditDefaultsOnly, Category = "Bomb|Explosion")
	FGameplayTag explosionDamageTag;
#pragma endregion

#pragma region FX
	UPROPERTY(EditDefaultsOnly, Category = "Bomb|FX")
	UNiagaraSystem* explosionVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Bomb|FX")
	USoundBase* explosionSound = nullptr;

	// 기폭(퓨즈) 시작 시 재생
	UPROPERTY(EditDefaultsOnly, Category = "Bomb|FX")
	USoundBase* fuseSound = nullptr;

	// 기폭 중 점멸에 쓸 머티리얼 — bodyEmitterMaterials와 같은 순서(이미터 인덱스)로 넣을 것.
	// 퓨즈 동안 bodyEmitterMaterials ↔ 이 배열을 번갈아 적용해 빨간 점멸을 만든다
	UPROPERTY(EditDefaultsOnly, Category = "Bomb|FX")
	TArray<UMaterialInterface*> fuseBlinkMaterials;

	// 점멸 간격 — 퓨즈 시작(Start)부터 폭발 직전(End)까지 선형으로 빨라짐
	UPROPERTY(EditAnywhere, Category = "Bomb|Explosion", meta = (ClampMin = "0.01"))
	float fuseBlinkIntervalStart = 0.30f;

	UPROPERTY(EditAnywhere, Category = "Bomb|Explosion", meta = (ClampMin = "0.01"))
	float fuseBlinkIntervalEnd = 0.06f;

	// 본체 파티클(ParticleSystemComponent)의 이미터별 머티리얼 오버라이드.
	// 배열 순서 = 이미터 인덱스. 비워두면 파티클 원본 머티리얼을 그대로 사용.
	//
	// 컴포넌트 디테일 패널의 Materials 항목에 넣어도 스폰된 인스턴스에는 전달되지 않아
	// (EmitterMaterials가 유실됨) BeginPlay에서 SetMaterial로 다시 적용해야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "Bomb|FX")
	TArray<UMaterialInterface*> bodyEmitterMaterials;
#pragma endregion

private:
	void UpdateTarget();

	// 돌진 — S자 위빙을 섞어 타겟으로 이동. BT로 옮길 경우 Task가 이 함수를 호출하면 됨
	void TickCharge(float DeltaTime);

	// 배회 — 홈 주변을 왕복. BT로 옮길 경우 Task가 이 함수를 호출하면 됨
	void TickPatrol(float DeltaTime);

	void PickPatrolPoint();

	// 본체 파티클 컴포넌트의 이미터 머티리얼을 통째로 교체.
	// 컴포넌트 템플릿에 넣은 값은 스폰 인스턴스에 전달되지 않으므로 런타임에 호출해야 함
	void ApplyEmitterMaterials(const TArray<UMaterialInterface*>& Materials);

	void ScheduleNextFuseBlink();

	UFUNCTION()
	void TickFuseBlink();

	void ApplyExplosionDamage();
	bool IsDead() const;

	UFUNCTION()
	void OnCapsuleHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	TWeakObjectPtr<AActor> targetActor;

	float searchTimer  = 0.f;
	bool  bFuseStarted = false;
	bool  bExploded    = false;

	// 위빙 위상 — 인스턴스마다 다른 값으로 시작해 여러 마리가 같은 궤적으로 겹치지 않게 함
	float weavePhase = 0.f;

	// 배회 기준점(BeginPlay 위치)과 현재 목표점
	FVector homeLocation       = FVector::ZeroVector;
	FVector patrolTargetPoint  = FVector::ZeroVector;
	float   patrolRepickTimer  = 0.f;
	float   patrolBobPhase     = 0.f;

	// 퓨즈 점멸 상태
	float fuseStartTime = 0.f;
	bool  bBlinkOn      = false;

	FTimerHandle fuseTimerHandle;
	FTimerHandle fuseBlinkTimerHandle;
};
