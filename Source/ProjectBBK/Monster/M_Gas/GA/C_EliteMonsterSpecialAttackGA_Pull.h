// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "C_EliteMonsterSpecialAttackGA_Pull.generated.h"

class AC_EliteBlackSphere;
class UAnimSequenceBase;
class UGameplayEffect;
class ACharacter;
class USoundBase;
struct FGameplayEventData;

/**
 * 엘리트 스페셜2 — 검은 구체 발사 + 끌어오기.
 * 던지기 애니(throwAnim) 재생 → 잠시 후 검은 구체(AC_EliteBlackSphere)를 플레이어 방향으로 발사.
 * 구체가 플레이어에 명중하면(SphereHit 이벤트) pullDelay(약 1초) 뒤 끌어오기 애니(pullAnim)를
 * 재생하며 플레이어를 몬스터 앞으로 부드럽게(보간) 끌어온다. 데미지는 구체 명중 시점에 처리된다.
 * 명중하지 못하면 maxWaitForHit 후 자동 종료한다.
 */
UCLASS()
class PROJECTBBK_API UC_EliteMonsterSpecialAttackGA_Pull : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UC_EliteMonsterSpecialAttackGA_Pull();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// BP에서 BP_EliteBlackSphere 지정
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Sphere")
	TSubclassOf<AC_EliteBlackSphere> sphereClass;

	// 던지기 애니 (마이그레이트한 Cast 시퀀스)
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Animation")
	UAnimSequenceBase* throwAnim = nullptr;

	// 끌어오기 애니 (마이그레이트한 Ultimate_Targeting_MSA 시퀀스)
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Animation")
	UAnimSequenceBase* pullAnim = nullptr;

	// 애니를 재생할 슬롯 이름 (ABP_EliteMonster의 DefaultSlot)
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Animation")
	FName animSlotName = TEXT("DefaultSlot");

	// 데미지 GE (GE_BasicDamage) — 구체 명중 시 적용
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Damage")
	TSubclassOf<UGameplayEffect> damageEffectClass;

	// DataTable Attack 스탯에 곱할 배율 (구체 데미지)
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Damage")
	float damageMultiplier = 1.0f;

	// 구체 발사는 Cast 애님의 AnimNotify(Event.Monster.Elite.ThrowRelease)가 트리거.
	// 이 값은 노티파이 미배치 대비 안전 폴백 지연(초) — 0이면 폴백 없이 노티파이만 사용.
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Sphere")
	float sphereSpawnFallbackDelay = 0.6f;

	// 구체 스폰 오프셋 (X=전방, Y=우측, Z=상단) — 손/가슴 높이 보정
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Sphere")
	FVector sphereSpawnOffset = FVector(80.f, 0.f, 100.f);

	// 명중 후 끌어오기 애니 재생까지 대기(초)
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Pull")
	float pullDelay = 1.0f;

	// 실제 드래그는 끌어오기 애니의 AnimNotify(Event.Monster.Elite.PullActivate)가 트리거.
	// 이 값은 노티파이 미배치 대비 폴백 지연(애니 시작 후 초) — 0이면 노티파이만 사용.
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Pull")
	float pullActivateFallbackDelay = 0.25f;

	// 끌어오기 보간 지속(초)
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Pull")
	float pullDuration = 0.5f;

	// 끌어온 뒤 몬스터 정면으로부터 멈출 거리(겹침 방지)
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Pull")
	float pullStopDistance = 150.f;

	// 구체가 명중하지 못하면 이 시간 뒤 어빌리티 자동 종료
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Sphere")
	float maxWaitForHit = 6.0f;

	// 검은 구체 발사 순간 재생 (EM_BlackOrbLaunch)
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Sound")
	USoundBase* launchSound = nullptr;

	// 끌어오기(드래그) 시작 순간 재생 (EM_Grab)
	UPROPERTY(EditDefaultsOnly, Category = "Special2|Sound")
	USoundBase* grabSound = nullptr;

private:
	UFUNCTION()
	void OnThrowRelease(FGameplayEventData Payload);

	UFUNCTION()
	void OnSphereHit(FGameplayEventData Payload);

	UFUNCTION()
	void OnPullActivate(FGameplayEventData Payload);

	void SpawnSphere();
	void StartPull();
	void BeginDrag();
	void TickPull();
	void FinishPull();
	void PlayMonsterAnim(UAnimSequenceBase* Anim);
	void SetPlayerMoveInputBlocked(bool bBlocked);
	void EndNow(bool bCancelled);

	FGameplayTag hitEventTag;
	FGameplayTag throwEventTag;
	FGameplayTag pullActivateTag;
	bool bSphereSpawned = false;
	bool bDragStarted = false;

	TWeakObjectPtr<ACharacter> pulledPlayer;
	FVector pullStart = FVector::ZeroVector;
	FVector pullTarget = FVector::ZeroVector;
	float   pullElapsed = 0.f;
	bool    bInputBlocked = false;

	static constexpr float pullTickInterval = 1.f / 60.f;

	FTimerHandle spawnTimerHandle;
	FTimerHandle pullDelayTimerHandle;
	FTimerHandle pullActivateTimerHandle;
	FTimerHandle pullTickTimerHandle;
	FTimerHandle timeoutTimerHandle;
};
