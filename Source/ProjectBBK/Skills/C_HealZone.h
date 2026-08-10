// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "C_HealZone.generated.h"

/**
 * 지면 AOE 힐 장판
 * UC_SpawnHealZoneAction이 스폰하며, 사용자 본인이 반경 안에 머무는 동안만
 * Instant 회복 GE를 일정 간격으로 반복 적용한다. 반경을 벗어나면 즉시 회복이 멈춘다.
 * Initialize() 호출 후 zoneLifetime이 지나면 자동 소멸.
 */
UCLASS()
class PROJECTBBK_API AC_HealZone : public AActor
{
	GENERATED_BODY()

public:
	AC_HealZone();

	/**
	 * UC_SpawnHealZoneAction에서 스폰 직후 호출.
	 * @param InInstigatorASC   치유 대상(사용자 본인)의 ASC
	 * @param InInstigatorActor 치유 대상(사용자 본인) — 이 액터만 필터링, 그 외는 무시
	 * @param InTickEffectClass Instant 회복 GE (Set by Caller Data.Heal)
	 * @param InRadius          존 반경
	 * @param InZoneLifetime    존 유지 시간(초) — 액터 자체의 수명, 회복 지속시간과 무관
	 * @param InTickInterval    회복 반복 간격(초)
	 * @param InHealPerTick     1틱당 회복량
	 */
	UFUNCTION(BlueprintCallable, Category = "HealZone")
	void Initialize(
		UAbilitySystemComponent* InInstigatorASC,
		AActor* InInstigatorActor,
		TSubclassOf<UGameplayEffect> InTickEffectClass,
		float InRadius,
		float InZoneLifetime,
		float InTickInterval,
		float InHealPerTick
	);

	/** Initialize() 완료 후 호출 — BP_HealZone에서 VFX 스케일 조정에 사용 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HealZone")
	void OnInitialized(float Radius);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HealZone")
	class USphereComponent* CollisionSphere;

private:
	TSubclassOf<UGameplayEffect> TickEffectClass;
	TWeakObjectPtr<UAbilitySystemComponent> InstigatorASC;
	TWeakObjectPtr<AActor> InstigatorActor;
	float TickInterval = 1.0f;
	float HealPerTick = 0.0f;

	FTimerHandle LifetimeHandle;
	FTimerHandle HealTickHandle;

	UFUNCTION()
	void OnBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	// 이미 회복 중이면 무시(중복 타이머 방지), 아니면 즉시 1틱 적용 후 반복 타이머 시작
	void StartHealing();

	// 반복 타이머 정지 — Instant GE라 "제거"할 활성 핸들이 없으므로 타이머를 멈추는 것으로 충분
	void StopHealing();

	void ApplyHealTick();

	void OnExpired();
};
