// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_SkillBase.h"
#include "C_RangedUltimate.generated.h"

/**
 * 원거리 캐릭터 궁극기.
 * 몽타주 재생 → AN_UltimateFire Notify(Event.Ultimate.Fire) 수신 시
 * 전방 Box 범위 몬스터에게 넉백 GE를 적용하고,
 * 데미지 GE는 Blueprint(OnNotifyReceived_BP)에서 처리한다.
 */
UCLASS()
class PROJECTBBK_API UC_RangedUltimate : public UC_SkillBase
{
	GENERATED_BODY()

public:
	UC_RangedUltimate();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// ────────────────────────────────────────
	// Box 판정 설정
	// ────────────────────────────────────────

	/** 전방 Box 깊이 절반 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RangedUltimate|Box")
	float BoxHalfExtentX = 600.f;

	/** Box 좌우 폭 절반 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RangedUltimate|Box")
	float BoxHalfExtentY = 150.f;

	/** Box 높이 절반 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RangedUltimate|Box")
	float BoxHalfExtentZ = 200.f;

	/** 캐릭터 중심에서 전방으로 Box 중심까지의 거리 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RangedUltimate|Box")
	float BoxOriginOffsetX = 600.f;

	// ────────────────────────────────────────
	// GE 설정
	// ────────────────────────────────────────

	/** 넉백 GameplayEffect (GE_Knockback 할당) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RangedUltimate|Effects")
	TSubclassOf<UGameplayEffect> KnockbackEffect;

	/** 넉백 강도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RangedUltimate|Effects")
	float KnockbackForce = 1200.f;

	/** AN_UltimateFire가 SendGameplayEventToActor로 전달하는 태그 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RangedUltimate|Events")
	FGameplayTag NotifyEventTag;

	// ────────────────────────────────────────
	// Blueprint 구현 이벤트
	// ────────────────────────────────────────

	/**
	 * Notify 수신 시 Blueprint에서 처리할 내용:
	 *   1. CurveTable 레벨 기반 데미지 조회 → DamageEffect 적용
	 *   2. DrawDebugBox
	 *
	 * @param HitActors  Box 범위 안에서 감지된 몬스터 목록
	 * @param BoxCenter  Box 중심 월드 좌표 (DrawDebugBox용)
	 * @param BoxExtent  Box 반크기 벡터 (DrawDebugBox용)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "RangedUltimate")
	void OnNotifyReceived_BP(
		const TArray<AActor*>& HitActors,
		FVector BoxCenter,
		FVector BoxExtent);

private:
	/** AN_UltimateFire Notify 수신 시 호출 */
	UFUNCTION()
	void HandleNotifyEvent(FGameplayEventData Payload);

	/** 몽타주 종료 콜백 — AddDynamic 함수명 레지스트리 정합을 위해 래핑 */
	UFUNCTION()
	void OnMontageEnded();
};
