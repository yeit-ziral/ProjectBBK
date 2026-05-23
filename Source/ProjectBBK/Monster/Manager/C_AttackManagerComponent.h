// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Data/AttackTypes.h"
#include "C_AttackManagerComponent.generated.h"

class AC_BaseMonster;
class UGameplayEffect;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTBBK_API UC_AttackManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UC_AttackManagerComponent();

	
	void Initialize(AC_BaseMonster* OwnerMonster); // ���Ͱ� BeginPlay���� �ڽ��� �Ѱܼ� �ʱ�ȭ

	bool CanAttack() const;
	void StartCooldown(float Seconds);

	bool CanSpecialAttack() const;
	void StartSpecialCooldown();

	bool DoNormalAttack();
	bool DoSpecialAttack();

	void DoSlam();

private:
	//private functions

	void DoMeleeNormalAttack();
	void ApplyGEDamage(ACharacter* Target, float DamageValue);

private:
	//private variable

	UPROPERTY()
	AC_BaseMonster* ownerMonster = nullptr;

#pragma region attack state(lastAttackTime, coolDownTime, traceRadius, debug)

	float lastAttackTime = -9999.0f;
	float lastSpecialAttackTime = -9999.0f;

	UPROPERTY(EditAnywhere, Category = "Attack")
	float coolDownTime = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Attack")
	float traceRadius = 60.0f;

	// 슬램 AOE 반경
	UPROPERTY(EditAnywhere, Category = "Attack")
	float slamRadius = 250.0f;

	// 스페셜 공격 데미지 배율 (기본 Attack 스탯 기준)
	UPROPERTY(EditAnywhere, Category = "Attack")
	float slamDamageMultiplier = 2.0f;

	// 넉백 강도
	UPROPERTY(EditAnywhere, Category = "Attack")
	float knockbackStrength = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> damageEffectClass;

	UPROPERTY(EditAnywhere, Category = "Attack")
	bool debug = false;
#pragma endregion



protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
