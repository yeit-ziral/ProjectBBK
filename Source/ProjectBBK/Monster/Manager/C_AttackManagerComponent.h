// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Data/AttackTypes.h"
#include "C_AttackManagerComponent.generated.h"

class AC_BaseMonster;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTBBK_API UC_AttackManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UC_AttackManagerComponent();

	
	void Initialize(AC_BaseMonster* OwnerMonster); // 몬스터가 BeginPlay에서 자신을 넘겨서 초기화

	bool CanAttack() const;
	void StartCooldown(float Seconds);

	bool DoNormalAttack();
	bool DoSpecialAttack();

	void DoSlam(); //DoBearSpecialAttackSlam을 외부에서 호출하기 위한 함수

private:
	//private functions


	void DoBearNormalAttack();
	void DoBearSpecialAttackJump();
	void DoBearSpecialAttackSlam();

private:
	//private variable

	UPROPERTY()
	AC_BaseMonster* ownerMonster = nullptr; // 소유 몬스터

#pragma region attack state(lastAttackTime, coolDownTime, traceRadius, debug)

	float lastAttackTime = 0.0f; // 마지막 공격 시점

	UPROPERTY(EditAnywhere, Category = "Attack")
	float coolDownTime = 1.0f; // 공격 쿨타임

	UPROPERTY(EditAnywhere, Category = "Attack")
	float traceRadius = 60.0f; // 트레이스 범위

	UPROPERTY(EditAnywhere, Category = "Attack")
	bool debug = false; // 디버그 표시 유무
#pragma endregion

#pragma region special attack state(slamDelaySeconds, jumpPower, slamRadius, knockbackStrength, knockupStrength)
	UPROPERTY(EditAnywhere, Category = "Special Attack")
	float slamDelaySeconds = 0.5f; // 점프 후 착지까지 대기 시간

	UPROPERTY(EditAnywhere, Category = "Special Attack")
	float jumpPower = 600.f; // 점프 높이

	UPROPERTY(EditAnywhere, Category = "Special Attack")
	float slamRadius = 350.f; // 공격 범위

	UPROPERTY(EditAnywhere, Category = "Special Attack")
	float knockbackStrength = 900.f; // 넉백 힘

	UPROPERTY(EditAnywhere, Category = "Special Attack")
	float knockupStrength = 250.f; // 위로 튕기는 힘


#pragma endregion



protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
