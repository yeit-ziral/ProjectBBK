// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_BaseMonster.h"
#include "C_RangedMonster.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API AC_RangedMonster : public AC_BaseMonster
{
	GENERATED_BODY()
public:
	AC_RangedMonster();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void RangedNormalAttack();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void RangedSpecialAttack();

protected:
	virtual void BeginPlay() override;

	// 애니메이션 (BearMonster와 동일한 방식)
	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* rangedNormalMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* rangedSpecialMontage = nullptr;

	// 발사체 (BP로 지정 가능)
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSubclassOf<AActor> projectileClass = nullptr;

	// 총구/손 소켓
	UPROPERTY(EditAnywhere, Category = "Projectile")
	FName muzzleSocketName = TEXT("Muzzle");

	// 발사체 스폰 오프셋 (소켓 없을 때 대비)
	UPROPERTY(EditAnywhere, Category = "Projectile")
	FVector muzzleOffset = FVector(50.f, 0.f, 50.f);

private:
	void SpawnProjectile();
};
