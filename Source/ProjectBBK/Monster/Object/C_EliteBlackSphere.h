// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "C_EliteBlackSphere.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UProjectileMovementComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * 엘리트 스페셜2 — 검은 구체 발사체.
 * 직선으로 날아가다 플레이어에 명중하면 GE_BasicDamage를 적용하고,
 * 시전 몬스터에게 hitEventTag GameplayEvent(Target=플레이어)를 보낸 뒤 소멸한다.
 * 명중 후 끌어오기(Pull)는 GA가 이벤트를 받아 처리한다.
 */
UCLASS()
class PROJECTBBK_API AC_EliteBlackSphere : public AActor
{
	GENERATED_BODY()

public:
	AC_EliteBlackSphere();

	// GA에서 호출 — ASC/시전자/데미지GE/데미지/방향/명중이벤트태그 주입 후 발사
	void InitSphere(UAbilitySystemComponent* InInstigatorASC, AActor* InInstigatorActor,
		TSubclassOf<UGameplayEffect> InDamageGE, float InDamage,
		const FVector& Direction, FGameplayTag InHitEventTag);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* collision;

	// BP에서 검은 구체 메시 + 어두운 머티리얼 지정 (C++ 하드코딩 금지)
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* sphereMesh;

	// BP에서 선택 트레일 Niagara 지정
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UNiagaraComponent* trailEffect;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UProjectileMovementComponent* projectileMovement;

	// 명중 못 하면 이 시간 뒤 자동 소멸
	UPROPERTY(EditDefaultsOnly, Category = "Sphere")
	float lifeSpanSeconds = 4.0f;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void ApplyDamageToTarget(AActor* Target);

	TWeakObjectPtr<UAbilitySystemComponent> instigatorASC;
	TWeakObjectPtr<AActor> instigatorActor;
	TSubclassOf<UGameplayEffect> damageGEClass;
	float damageValue = 0.f;
	FGameplayTag hitEventTag;
	bool bAlreadyHit = false;
};
