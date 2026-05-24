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

class UNiagaraSystem;
class AC_ExpOrb;

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

	// true로 설정하면 Tick마다 그로기 자동 누적 (기본 비활성화)
	UPROPERTY(EditDefaultsOnly, Category = "Groggy")
	bool bAutoAccumulateGroggy = false;

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
	UC_GroggyComponent*          GetGroggyComponent()     const { return groggyComponent; }

	bool  IsRepositionEnabled()       const;
	float GetRepositionDesiredRange() const;
	float GetRepositionMinRange()     const;
	float GetRepositionSpeed()        const;
	float GetRepositionStrafeWeight() const;
	float GetRepositionBand()         const;
	float GetRepositionFlipInterval() const;

	virtual bool CanAutoAttack() const;
	virtual bool IsPlayingAttackAnimation() const { return false; }

	void TakeHitReaction();
	void StartHitFlash();
#pragma endregion

#pragma region HitReaction
protected:
	UPROPERTY(EditDefaultsOnly, Category = "HitReaction")
	UAnimMontage* hitReactionMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "HitReaction")
	UMaterialInterface* hitFlashMaterial = nullptr;

private:
	void HitFlashTick();
	FTimerHandle hitFlashTimerHandle;
	int32 hitFlashStep = 0;
#pragma endregion

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

#pragma region Death
public:
	// ANC_DeathVFX AnimNotify에서 호출 — 안개 VFX 스폰 + 메시 숨김 + 소멸 타이머 시작
	void OnDeathMontageVFXPoint();

protected:
	// OnMonsterDeath 델리게이트에서 호출 — AI·이동·콜리전 정지, 몽타주 재생
	virtual void ExecuteDeathSequence();

	// deathDestroyDelay 후 호출
	void DestroyAfterDeath();

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	UAnimMontage* deathMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	UNiagaraSystem* deathFogVFX = nullptr;

	// 안개 VFX 스폰 후 액터 소멸까지 대기 시간 (VFX 지속 시간보다 길어야 함)
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	float deathDestroyDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Reward")
	TSubclassOf<AC_ExpOrb> ExpOrbClass;

public:
	// Reposition 스트레이프 상태 — BT 세션 간 유지 (C_BTTaskReposition이 관리)
	int8  repositionStrafeSign   = 1;
	float repositionNextFlipTime = -1.f;   // -1: 미초기화 sentinel

private:
	FTimerHandle deathDestroyTimerHandle;
	bool bDeathVFXTriggered = false;
#pragma endregion
};
