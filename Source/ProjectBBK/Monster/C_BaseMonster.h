// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "C_BaseMonster.generated.h"

class UDataTable;
class UC_AttackManagerComponent;
struct FMonsterData;

UCLASS()
class PROJECTBBK_API AC_BaseMonster : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AC_BaseMonster();



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:	 //protected variables

#pragma region data(rowName, monsterTable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FName rowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TSoftObjectPtr<UDataTable> monsterTable;
#pragma endregion

#pragma region stats(monsterId, hp, attack, movespeed, attackRange, attackCooldown)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 monsterId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 hp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 attack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float moveSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float attackRange;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float attackCooldown;
#pragma endregion

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manager")
	UC_AttackManagerComponent* attackManager = nullptr;

public: // public functions
	void ApplyData(const struct FMonsterData& Data);

	int32 GetMonsterID() const { return monsterId; }

	int32 GetHP() const { return hp; }

	int32 GetAttack() const { return attack; }

	float GetAttackCooldown() const { return attackCooldown; }

	float GetAttackRange() const { return attackRange; }

	UC_AttackManagerComponent* GetAttackManager() const { return attackManager; }

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
