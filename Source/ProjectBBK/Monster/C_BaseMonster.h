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
#include "UI/C_MonsterHPWidgetBase.h"

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
	virtual void PostInitializeComponents() override;

protected:	 //protected variables

#pragma region data(rowName, monsterTable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FName rowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TSoftObjectPtr<UDataTable> monsterTable;
#pragma endregion

#pragma region stats(monsterId)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 monsterId;

#pragma endregion

#pragma region Gas(MonsterASC, MonsterAttributeSet, InitializeAttributesFromDataTable, Tag)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UC_MonsterASC* monsterASC;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> gamePlayAbilities;

	UPROPERTY()
	UC_MonsterAttributeSet* monsterAttributeSet;

	FGameplayTag monsterTypeTag;

	void ApplyMonsterTypeTag();

	void InitializeAttributesFromDataTable();

	void PrintMonsterTags();

#pragma endregion

#pragma region UI(HpWidgetComponent, MonsterHpWidget)
	UPROPERTY(VisibleAnywhere, Category = "UI")
	UWidgetComponent* HpWidgetComponent;

	UPROPERTY()
	UC_MonsterHPWidgetBase* MonsterHpWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> hpWidgetClass;

	void InitializeMonsterHpWidget();
	void InitializeHpWidgetClass();
	virtual TSubclassOf<UUserWidget> GetHpWidgetClass() const;
#pragma endregion

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Manager")
	UC_AttackManagerComponent* attackManager = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	UBehaviorTree* behaviorTree;

public: // public functions

	int32 GetMonsterID() const { return monsterId; }

	int32 GetmaxHP() const { return monsterAttributeSet->GetmaxHP(); }

	int32 GetcurHP() const { return monsterAttributeSet->GetcurHP(); }

	int32 GetmaxGroggy() const { return monsterAttributeSet->GetmaxGroggy(); }

	int32 GetcurGroggy() const { return monsterAttributeSet->GetcurGroggy(); }

	int32 GetAttack() const { return monsterAttributeSet->Getattack(); }

	float GetAttackCooldown() const { return monsterAttributeSet->GetnormalCooldown(); }

	float GetAttackRange() const { return monsterAttributeSet->GetattackRange(); }

	UC_AttackManagerComponent* GetAttackManager() const { return attackManager; }

	UBehaviorTree* GetBehaviorTree() const { return behaviorTree; }

	UC_MonsterASC* GetMonsterASC() const { return monsterASC; }

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


};
