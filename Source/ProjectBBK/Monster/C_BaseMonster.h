// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "M_Gas/C_MonsterAttributeSet.h"
#include "M_Gas/C_MonsterASC.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/WidgetComponent.h"
#include "GameplayTagContainer.h"

#include "C_BaseMonster.generated.h"

class UC_AttackManagerComponent;
class UC_MonsterDataComponent;
class UC_GroggyComponent;
class UC_MonsterHPDisplayComponent;

UCLASS()
class PROJECTBBK_API AC_BaseMonster : public ACharacter
{
	GENERATED_BODY()

public:
	AC_BaseMonster();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;

protected:

#pragma region GAS(MonsterASC, MonsterAttributeSet, GameplayAbilities)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UC_MonsterASC* monsterASC;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> gamePlayAbilities;

	UPROPERTY()
	UC_MonsterAttributeSet* monsterAttributeSet;

	// 파생 클래스에서 BeginPlay에 설정 후 ApplyMonsterTypeTag() 호출
	FGameplayTag monsterTypeTag;
	void ApplyMonsterTypeTag();
#pragma endregion

#pragma region Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manager")
	UC_AttackManagerComponent* attackManager = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Data")
	UC_MonsterDataComponent* dataComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Groggy")
	UC_GroggyComponent* groggyComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UC_MonsterHPDisplayComponent* hpDisplayComponent = nullptr;

	// Scene 컴포넌트 — 생성자에서 루트에 부착, HPDisplayComponent가 관리
	UPROPERTY(VisibleAnywhere, Category = "UI")
	UWidgetComponent* HpWidgetComponent;
#pragma endregion

#pragma region AI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBehaviorTree* behaviorTree;
#pragma endregion

public:

#pragma region Getters
	int32 GetMonsterID()      const;
	FName GetRowName()        const;  // BossMonster 호환용

	int32 GetmaxHP()          const { return monsterAttributeSet->GetmaxHP(); }
	int32 GetcurHP()          const { return monsterAttributeSet->GetcurHP(); }
	int32 GetmaxGroggy()      const { return monsterAttributeSet->GetmaxGroggy(); }
	int32 GetcurGroggy()      const { return monsterAttributeSet->GetcurGroggy(); }
	int32 GetAttack()         const { return monsterAttributeSet->Getattack(); }
	float GetAttackCooldown() const { return monsterAttributeSet->GetnormalCooldown(); }
	float GetSpecialCooldown()const { return monsterAttributeSet->GetspecialCooldown(); }
	float GetAttackRange()    const { return monsterAttributeSet->GetattackRange(); }

	UC_AttackManagerComponent*   GetAttackManager()       const { return attackManager; }
	UBehaviorTree*               GetBehaviorTree()        const { return behaviorTree; }
	UC_MonsterASC*               GetMonsterASC()          const { return monsterASC; }
	UC_MonsterAttributeSet*      GetMonsterAttributeSet() const { return monsterAttributeSet; }
	UC_MonsterDataComponent*     GetDataComponent()       const { return dataComponent; }
	UC_MonsterHPDisplayComponent*GetHPDisplayComponent()  const { return hpDisplayComponent; }
#pragma endregion

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
