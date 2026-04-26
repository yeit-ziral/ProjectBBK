// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_GridLaser.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UStaticMeshComponent;
class UC_MonsterASC;
class UGameplayEffect;

UCLASS()
class PROJECTBBK_API AC_GridLaser : public AActor
{
	GENERATED_BODY()

public:
	AC_GridLaser();

protected:
	virtual void BeginPlay() override;

public:
	void Initialize(UC_MonsterASC* InInstigatorASC, AActor* InInstigatorActor,
		TSubclassOf<UGameplayEffect> InDamageEffectClass, float InDamageAmount,
		float InBeamLength, float InBeamWidth, float InPreviewTime, float InFireDuration);

	void CancelLaser();

private:
	UFUNCTION()
	void OnFireStart();

	UFUNCTION()
	void OnFireEnd();

	void ApplyDamageTick();
	void ApplyDamageTo(AActor* Target);

	UFUNCTION()
	void OnHitBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY()
	USceneComponent* root;

	UPROPERTY(EditAnywhere, Category = "GridLaser")
	UBoxComponent* damageBox;

	UPROPERTY(EditAnywhere, Category = "GridLaser")
	UNiagaraComponent* beamFX;

	UPROPERTY(EditAnywhere, Category = "GridLaser")
	UStaticMeshComponent* previewPlane;

	UPROPERTY()
	TWeakObjectPtr<UC_MonsterASC> instigatorASC;

	UPROPERTY()
	TWeakObjectPtr<AActor> instigatorActor;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> damageEffectClass;

	float damageAmount = 500.f;
	float previewTime = 1.5f;
	float fireDuration = 2.0f;

	bool bCancelled = false;
	bool bFiring = false;

	FTimerHandle fireStartTimer;
	FTimerHandle fireEndTimer;
	FTimerHandle damagePulseTimer;
};
