// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_BossBeam.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;

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
	float rotateSpeed = 90.f;

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

private:
	UPROPERTY()
	USceneComponent* root;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* previewPlane;

	UPROPERTY(EditAnywhere)
	UNiagaraComponent* beam;

	bool bIsPreview = true;

	FTimerHandle previewTimerHandle;

};
