// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_Portal.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class USceneComponent;
class AC_BasePlayerCharactor;

// 순환 참조 방지를 위해 클래스 전방 선언 후 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPortalEntered, AC_Portal*, Portal);

UCLASS()
class PROJECTBBK_API AC_Portal : public AActor
{
	GENERATED_BODY()

public:
	AC_Portal();

	// GameMode에서 바인딩 — 플레이어 진입 시 브로드캐스트
	UPROPERTY(BlueprintAssignable, Category = "Portal")
	FOnPortalEntered OnPortalEntered;

	UFUNCTION(BlueprintCallable, Category = "Portal")
	void ActivatePortal();

	UFUNCTION(BlueprintCallable, Category = "Portal")
	void DeactivatePortal();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPortalOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* RootComp;

	// 기본 크기: X=20(깊이), Y=300(너비), Z=400(높이) — 문 형태
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* PortalCollision;

	// 나이아가라 에셋은 BP_Portal에서 할당
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UNiagaraComponent* PortalEffect;

	bool bIsActivated = false;
};
