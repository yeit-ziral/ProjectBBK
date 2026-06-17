// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_BaseMonster.h"
#include "C_RangedMonster.generated.h"

class UGameplayEffect;

/**
 *
 */
UCLASS()
class PROJECTBBK_API AC_RangedMonster : public AC_BaseMonster
{
	GENERATED_BODY()
public:
	AC_RangedMonster();

	virtual bool CanAutoAttack() const override;
	virtual bool IsPlayingAttackAnimation() const override;

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void RangedNormalAttack();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void RangedSpecialAttack();

	// BT에서 호출 — 스페셜 쿨타임 여부에 따라 스페셜/노말 자동 선택
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void RangedAutoAttack();

protected:
	virtual void BeginPlay() override;

	// �ִϸ��̼� (MeleeMonster�� ������ ���)
	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* rangedNormalMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* rangedSpecialMontage = nullptr;

	// 일반 공격 발사체
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSubclassOf<AActor> projectileClass = nullptr;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	FName muzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditAnywhere, Category = "Projectile")
	FVector muzzleOffset = FVector(50.f, 0.f, 50.f);

	// 노말 공격 GA
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> normalAttackGAClass;

	// 스페셜 공격 GA (BP에서 BPC_RangedMonsterSpecialAttackGA 할당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> specialAttackGAClass;

private:
	void SpawnProjectile();
};
