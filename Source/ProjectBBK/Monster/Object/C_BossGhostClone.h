// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_BossGhostClone.generated.h"

class USkeletalMeshComponent;
class UWidgetComponent;
class USphereComponent;
class UC_BossGridLaserPatternGA;
class AC_GridLaser;

UCLASS()
class PROJECTBBK_API AC_BossGhostClone : public AActor
{
	GENERATED_BODY()

public:
	AC_BossGhostClone();

	virtual void Tick(float DeltaTime) override;

public:
	void Initialize(UC_BossGridLaserPatternGA* InOwningGA, const FVector& InLaserDirection);
	// Duration: chargeDuration에 맞게 재생 속도를 조절해 슬로우 모션처럼 재생
	void PlayCharge(UAnimMontage* ChargeMontage, float Duration);
	void PlayFire(UAnimMontage* FireMontage);
	void SetMarked(bool bMarked);

	FVector laserDirection;
	bool bCancelled = false;

	UPROPERTY()
	TWeakObjectPtr<AC_GridLaser> pairedLaser;

private:
	UFUNCTION()
	void OnHitSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(EditAnywhere, Category = "GhostClone")
	USkeletalMeshComponent* ghostMesh;

	UPROPERTY(EditAnywhere, Category = "GhostClone")
	UWidgetComponent* markWidget;

	UPROPERTY(EditAnywhere, Category = "GhostClone")
	USphereComponent* hitDetectSphere;

	UPROPERTY()
	TWeakObjectPtr<UC_BossGridLaserPatternGA> owningGA;

	bool bIsMarked = false;
};
