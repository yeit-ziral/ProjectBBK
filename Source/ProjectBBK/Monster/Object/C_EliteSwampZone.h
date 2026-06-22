// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_EliteSwampZone.generated.h"

class USceneComponent;
class UDecalComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UAbilitySystemComponent;
class UGameplayEffect;
class UMaterialInterface;

/**
 * 엘리트 스페셜1 — 검은 수렁 장판.
 * 몬스터에 부착되어 따라다니며, 일정 시간 동안 주기적으로
 * 반경 안의 플레이어에게 최대 체력 비율 데미지를 가한다.
 * 데미지/판정은 C_BossDangerZone 패턴을 따르되, "안에 있을 때" 데미지(조건 반전).
 */
UCLASS()
class PROJECTBBK_API AC_EliteSwampZone : public AActor
{
	GENERATED_BODY()

public:
	AC_EliteSwampZone();

	// GA에서 호출 — ASC/반경/체력비율/틱주기/지속시간 설정 후 타이머 시작
	UFUNCTION(BlueprintCallable, Category = "SwampZone")
	void InitSwampZone(
		UAbilitySystemComponent* InOwnerASC,
		float InRadius        = 300.f,
		float InHealthPercent = 0.05f,
		float InTickRate      = 1.0f,
		float InDuration      = 5.0f);

	UFUNCTION(BlueprintCallable, Category = "SwampZone")
	void StopZone();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "SwampZone")
	USceneComponent* sceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "SwampZone")
	UDecalComponent* decalComponent;

	// 입체감용 늪 가스/안개 — 첫 번째 인스턴스(나머지는 InitSwampZone에서 동적 스폰)
	UPROPERTY(VisibleAnywhere, Category = "SwampZone")
	UNiagaraComponent* mistComponent;

	// 디스크 전역에 흩뿌린 추가 안개 인스턴스 (GC 참조 유지용)
	UPROPERTY(Transient)
	TArray<UNiagaraComponent*> mistInstances;

	// BP에서 M_EliteSwamp 지정 (C++ 하드코딩 금지)
	UPROPERTY(EditDefaultsOnly, Category = "SwampZone")
	UMaterialInterface* decalMaterial;

	// BP에서 Niagara(늪 가스) 지정 — 미지정 시 데칼만
	UPROPERTY(EditDefaultsOnly, Category = "SwampZone")
	UNiagaraSystem* mistEffect;

	// 장판에 흩뿌릴 안개 인스턴스 개수 (많을수록 균일·고비용)
	UPROPERTY(EditDefaultsOnly, Category = "SwampZone", meta = (ClampMin = "1"))
	int32 mistInstanceCount = 7;

	// 인스턴스 분포 반경 = 장판 반경 × 이 값 (0~1)
	UPROPERTY(EditDefaultsOnly, Category = "SwampZone", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float mistCoverage = 0.85f;

	// 안개 인스턴스 1개당 스케일 (장판이 클수록 키워서 빈틈 메움)
	UPROPERTY(EditDefaultsOnly, Category = "SwampZone")
	float mistScaleMultiplier = 1.5f;

	// BP에서 GE_BasicDamage 지정
	UPROPERTY(EditDefaultsOnly, Category = "SwampZone")
	TSubclassOf<UGameplayEffect> damageEffectClass;

private:
	UFUNCTION()
	void TickDamage();

	void DestroyZone();

	TWeakObjectPtr<UAbilitySystemComponent> ownerASC;
	float radius        = 300.f;
	float healthPercent = 0.05f;

	FTimerHandle damageTimerHandle;
	FTimerHandle lifetimeTimerHandle;
};
