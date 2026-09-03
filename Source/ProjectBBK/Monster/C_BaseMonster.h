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
class USoundBase;
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

	UC_AttackManagerComponent*   GetAttackManager()       const;
	UBehaviorTree*               GetBehaviorTree()        const { return behaviorTree; }
	UC_MonsterASC*               GetMonsterASC()          const { return monsterASC; }
	UC_MonsterAttributeSet*      GetMonsterAttributeSet() const { return monsterAttributeSet; }
	UC_MonsterDataComponent*     GetDataComponent()       const { return dataComponent; }
	UC_MonsterHPDisplayComponent*GetHPDisplayComponent()  const { return hpDisplayComponent; }
	UC_GroggyComponent*          GetGroggyComponent()     const { return groggyComponent; }

	// 스페셜 GA 슬롯 1/2의 사거리 (DataTable Special1Range/Special2Range, 미지정 시 AttackRange)
	float GetSpecial1Range()          const;
	float GetSpecial2Range()          const;

	// 스페셜 GA 슬롯 1/2의 쿨다운 (DataTable Special1Cooldown/Special2Cooldown, 미지정 시 SpecialCooldown)
	float GetSpecial1Cooldown()       const;
	float GetSpecial2Cooldown()       const;

	bool  IsRepositionEnabled()       const;
	float GetRepositionDesiredRange() const;
	float GetRepositionMinRange()     const;
	float GetRepositionSpeed()        const;
	float GetRepositionStrafeWeight() const;
	float GetRepositionBand()         const;
	float GetRepositionFlipInterval() const;

	virtual bool CanAutoAttack() const;
	virtual bool IsPlayingAttackAnimation() const { return false; }

	// 그로기 중이거나 공격 애니메이션 재생 중에는 멈추는 시계(초).
	// 모든 몬스터의 공격 쿨타임은 GetWorld()->GetTimeSeconds() 대신 이 값을 기준으로 재야
	// "그로기/공격 모션 동안 쿨타임이 흐르지 않는다"가 몬스터 종류와 무관하게 성립한다.
	// (쿨타임을 UC_AttackManagerComponent가 관리하든 몬스터 클래스가 직접 관리하든 동일)
	float GetAttackClock() const { return attackClock; }

	// 공격 시계를 멈춰야 하는 상태인지 — 기본: State.Groggy 태그 또는 IsPlayingAttackAnimation()
	virtual bool IsAttackCooldownPaused() const;

	// 해당 GA가 지금 활성 중인지. 공격 몽타주를 GA 블루프린트가 내부에서 재생해
	// C++에 몽타주 레퍼런스가 없는 몬스터(엘리트·보스)의 IsPlayingAttackAnimation() 구현용.
	bool IsAbilityActive(TSubclassOf<UGameplayAbility> AbilityClass) const;

	// BT의 UC_BTTaskMeleeAutoAttack이 호출하는 공통 자동공격 진입점.
	// 몬스터별 XxxAutoAttack()을 여기서 override로 연결하면 Task 클래스를 새로 만들 필요가 없다.
	// 실행했으면 true, 쿨타임/조건 미충족이면 false.
	virtual bool TryAutoAttack() { return false; }

	// 들어온 데미지를 몬스터별로 가공할 기회 — UC_MonsterAttributeSet::PostGameplayEffectExecute에서
	// 방어력(defense) 감산 **이전**에 호출된다. 기본 구현은 원본 값을 그대로 반환.
	// bTrueDamage가 true면 방어력을 무시하는 DoT/상태이상 데미지 경로.
	// DamageInstigator는 EffectContext 기준이라 null일 수 있음.
	virtual float ModifyIncomingDamage(float RawDamage, AActor* DamageInstigator, bool bTrueDamage) { return RawDamage; }

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

	// 사망 시퀀스 시작 순간 재생 — 몬스터마다 BP에서 지정. 미지정이면 무음
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	USoundBase* deathSound = nullptr;

	// 안개 VFX 스폰 후 액터 소멸까지 대기 시간 (VFX 지속 시간보다 길어야 함)
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	float deathDestroyDelay = 2.0f;

	// 사망 후 이 시간이 지나면 몽타주 끝을 기다리지 않고 VFX + 메시 숨김을 강제 실행
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	float deathVFXDelay = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Reward")
	TSubclassOf<AC_ExpOrb> ExpOrbClass;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	int32 level = 1;

	// Reposition 스트레이프 상태 — BT 세션 간 유지 (C_BTTaskReposition이 관리)
	int8  repositionStrafeSign   = 1;
	float repositionNextFlipTime = -1.f;   // -1: 미초기화 sentinel

private:
	// IsAttackCooldownPaused()가 false인 프레임에만 DeltaTime만큼 증가 (Tick에서 갱신)
	float attackClock = 0.0f;

	FTimerHandle deathDestroyTimerHandle;
	FTimerHandle deathVFXTimerHandle;
	bool bDeathVFXTriggered = false;
#pragma endregion
};
