// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectBBK/PlayerCharacter/C_BasePlayerCharactor.h"
#include "C_RangeCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedDelegate, float, CurrentAmmo, float, MaxAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReloadStartedDelegate, float, ReloadTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadFinishedDelegate);

UCLASS()
class PROJECTBBK_API AC_RangeCharacter : public AC_BasePlayerCharactor
{
	GENERATED_BODY()

public:
	AC_RangeCharacter(const class FObjectInitializer &ObjectInitializer);

	// 감지 범위 내 가장 가까운 살아있는 몬스터 반환 (없으면 nullptr)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RangeCharacter|Combat")
	class AC_BaseMonster *GetHighestPriorityTarget() const;

	// 타겟 방향으로 캐릭터를 즉시 회전 (XY 평면)
	UFUNCTION(BlueprintCallable, Category = "RangeCharacter|Combat")
	void FaceTarget(AActor *Target);

	UFUNCTION(BlueprintPure, Category = "RangeCharacter|Ammo")
	float GetAmmo() const;

	UFUNCTION(BlueprintPure, Category = "RangeCharacter|Ammo")
	float GetMaxAmmo() const;

	UFUNCTION(BlueprintPure, Category = "RangeCharacter|Ammo")
	float GetReloadTime() const;

	UFUNCTION(BlueprintCallable, Category = "RangeCharacter|Ammo")
	void RefillAmmo();

	UFUNCTION(BlueprintCallable, Category = "RangeCharacter|Ammo")
	void NotifyReloadStarted();

	UFUNCTION(BlueprintCallable, Category = "RangeCharacter|Ammo")
	void NotifyReloadFinished();

protected:
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	void RegisterAmmoDelegate();
	void OnAmmoChangedInternal(const struct FOnAttributeChangeData& Data);
	void TryAutoReload();

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

	// 감지 반경 (에디터에서 조정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RangeCharacter|Combat")
	float detectionRange = 600.f;

	UPROPERTY(BlueprintAssignable, Category = "RangeCharacter|Ammo")
	FOnAmmoChangedDelegate OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "RangeCharacter|Ammo")
	FOnReloadStartedDelegate OnReloadStarted;

	UPROPERTY(BlueprintAssignable, Category = "RangeCharacter|Ammo")
	FOnReloadFinishedDelegate OnReloadFinished;

protected:
	bool bAmmoDelegateRegistered = false;
};
