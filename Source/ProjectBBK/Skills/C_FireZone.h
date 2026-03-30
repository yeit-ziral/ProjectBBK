// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "C_FireZone.generated.h"

/**
 * 지면 AOE 화염 존
 * GA_Ablaze가 스폰하며, 범위 내 진입하는 몬스터에게 GE_Ablaze를 적용한다.
 * Initialize() 호출 후 lifetime이 지나면 자동 소멸.
 */
UCLASS()
class PROJECTBBK_API AC_FireZone : public AActor
{
	GENERATED_BODY()

public:
	AC_FireZone();

	/**
	 * GA_Ablaze에서 스폰 직후 호출.
	 * @param InInstigatorASC  플레이어 ASC (GE 적용 및 마나 충전 컨텍스트용)
	 * @param InInstigatorActor 플레이어 Actor
	 * @param InEffectClass    적용할 GE (GE_Ablaze)
	 * @param InRadius         존 반경
	 * @param InLifetime       존 유지 시간 (초)
	 */
	UFUNCTION(BlueprintCallable, Category = "FireZone")
	void Initialize(
		UAbilitySystemComponent* InInstigatorASC,
		AActor* InInstigatorActor,
		TSubclassOf<UGameplayEffect> InEffectClass,
		float InRadius,
		float InLifetime,
		float InDamageAmount
	);

	/** Initialize() 완료 후 호출 — BP_FireZone에서 VFX 스케일 조정에 사용 */
	UFUNCTION(BlueprintImplementableEvent, Category = "FireZone")
	void OnInitialized(float Radius);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FireZone")
	class USphereComponent* CollisionSphere;

private:
	TSubclassOf<UGameplayEffect> EffectClass;
	TWeakObjectPtr<UAbilitySystemComponent> InstigatorASC;
	TWeakObjectPtr<AActor> InstigatorActor;
	float DamageAmount = 0.0f;

	FTimerHandle LifetimeHandle;

	UFUNCTION()
	void OnBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	void ApplyEffectToTarget(AActor* TargetActor);

	void OnExpired();
};
