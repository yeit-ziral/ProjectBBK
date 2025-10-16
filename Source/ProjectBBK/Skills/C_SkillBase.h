// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SkillData.h"
#include "C_SkillBase.generated.h"

class AActor;
class APlayerCharacter;
class UCooldownManager;

//Parent class of all skills
UCLASS(Blueprintable, Abstract)
class PROJECTBBK_API UC_SkillBase : public UObject
{
	GENERATED_BODY()
	
public:
	UC_SkillBase();

	UFUNCTION(BlueprintCallable, Category = "Skill")
	virtual void InitializeSkill(AActor* InOwner, const FSkillData& InSkillData);

	UFUNCTION(BlueprintCallable, Category = "Skill")
	virtual bool CanUseSkill() const;

	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool CastSkill();

	UFUNCTION(BlueprintCallable, Category = "Skill")
	virtual void UpdateSkill(float deltaTime);

	UFUNCTION(BlueprintCallable, Category = "Skill")
	virtual void CancelSkill();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Skill")
	void ExecuteSkill();
	virtual void ExecuteSkill_Implementation() PURE_VIRTUAL(UC_SkillBase::ExecuteSkill_Implementation, );

	UFUNCTION(BlueprintNativeEvent, Category = "Skill")
	void OnSkillStart();
	virtual void OnSkillStart_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Skill")
	void OnSkillEnd();
	virtual void OnSkillEnd_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Skill")
	void OnSkillCancelled();
	virtual void OnSkillCancelled_Implementation();

	UFUNCTION(BlueprintCallable, Category = "Skill|Animation")
	void PlaySkillAnimation();

	UFUNCTION(BlueprintCallable, Category = "Skill|VFX")
	void SpawnSkillEffect(int32 EffectType, FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Skill|Sound")
	void PlaySkillSound(int32 SoundType);

	UFUNCTION(BlueprintCallable, Category = "Skill|Cooldown")
	void StartCooldown();

public:
	UFUNCTION(BlueprintPure, Category = "Skill")
	bool IsOnCooldown() const { return currentCooldown > 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetCurrentCooldown() const { return currentCooldown; }

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetMaxCooldown() const { return skillData.cooldown; }

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetCooldownPercent() const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	ESkillState GetSkillState() const { return currentState; }

	UFUNCTION(BlueprintPure, Category = "Skill")
	FSkillData GetSkillData() const { return skillData; }

	UFUNCTION(BlueprintPure, Category = "Skill")
	AActor* GetOwner() const { return owner; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	AActor* owner;

	// 스킬 데이터
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	FSkillData skillData;

	// 현재 상태
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	ESkillState currentState;

	// 쿨타임
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	float currentCooldown;

	// 시전 중 여부
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	bool bIsCasting;

	// 시전 시간 (애니메이션 길이 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
	float castTime;

	// 현재 시전 경과 시간
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	float currentCastTime;
};
