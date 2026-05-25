// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_BossBeam.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class PROJECTBBK_API AC_BossBeam : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AC_BossBeam();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float rotateSpeed = 63.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* ownerBoss;

	UPROPERTY(EditAnywhere)
	float previewTime = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beam")
	float lifeTime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beam")
	float distanceFromBoss = 150.f;

	UFUNCTION()
	void SwitchToBeam();

	// GA가 타이밍을 직접 제어할 때 내부 자동 전환 타이머를 비활성화
	void DisableAutoSwitch();

	// GAS 데미지 파라미터 설정 — SwitchToBeam() 전에 호출
	void InitBeam(UAbilitySystemComponent* InInstigatorASC,
	              TSubclassOf<UGameplayEffect> InDamageGEClass,
	              float InDamageValue,
	              float InBeamRange      = 2000.f,
	              float InDamageTickRate = 0.5f);

private:
	UPROPERTY()
	USceneComponent* root;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* previewPlane;

	UPROPERTY(EditAnywhere)
	UNiagaraComponent* beam;

	bool bIsPreview = true;

	FTimerHandle previewTimerHandle;
	FTimerHandle damageTickHandle;

	void ApplyBeamDamage();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	TWeakObjectPtr<UAbilitySystemComponent> instigatorASC;
	TSubclassOf<UGameplayEffect> damageGEClass;
	float damageValue    = 0.f;
	float beamRange      = 2000.f;
	float damageTickRate = 0.5f;
};
