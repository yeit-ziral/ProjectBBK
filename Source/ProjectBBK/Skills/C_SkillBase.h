// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "../GAS/Abilities/C_CharacterGA.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "SkillData.h"
#include "C_SkillBase.generated.h"

/**
 * 모든 스킬의 베이스 클래스
 * DT_SkillData를 읽어서 스킬을 자동으로 구성
 * Generic Cooldown 시스템 사용
 */
UCLASS()
class PROJECTBBK_API UC_SkillBase : public UC_CharacterGA
{
	GENERATED_BODY()

public:
	UC_SkillBase();

	/** Get Skill Data (loads if not cached) */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool GetSkillData(FSkillData& OutSkillData);

protected:
	// ========================================
	// Skill Configuration
	// ========================================

	/** Skill Data Table Reference */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Config")
	UDataTable* SkillDataTable;

	/** Skill Row Name in DataTable */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Config")
	FName SkillRowName;

	/** Generic Cooldown Gameplay Effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Config")
	TSubclassOf<UGameplayEffect> GenericCooldownGE;

	/** Cooldown Tag for this skill (e.g., Cooldown.Skill.SpeedBuff) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Config")
	FGameplayTag CooldownTag;

	// ========================================
	// Skill Data
	// ========================================

	/** Cached Skill Data */
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	FSkillData CachedSkillData;

	/** Is Skill Data Loaded? */
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	bool bSkillDataLoaded;

	// ========================================
	// Core Functions
	// ========================================

	/** Load Skill Data from DataTable */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool LoadSkillData();

	/** Apply Generic Cooldown using DataTable cooldown value */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void ApplySkillCooldown();

	/** Apply Generic Cooldown with custom duration */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void ApplyGenericCooldown(float CooldownDuration);

	/** Check if skill is on cooldown */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool IsOnCooldown() const;

	/** Apply Gameplay Effects to Self from Skill Data */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void ApplySkillEffects();

	/** Apply EffectsToTarget from Skill Data to given actors (즉시 AoE 스킬용) */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void ApplySkillEffectsToTargets(const TArray<AActor*>& Targets);

	/** Play Skill Animation */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void PlaySkillAnimation();

	/** Spawn Skill VFX */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void SpawnSkillVFX();

	/** Play Skill Sound */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void PlaySkillSound();

	// ========================================
	// Overrides
	// ========================================

	/** Override to add cooldown check */
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags
	) const override;

	/** Called when ability is given to ASC */
	virtual void OnGiveAbility(
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilitySpec& Spec
	) override;

public:
	// ========================================
	// Event Dispatchers (for UI)
	// ========================================

	/** Called when cooldown starts */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
		FOnCooldownStartedSignature,
		float, CooldownDuration,
		FGameplayTag, SkillTag,
		FGameplayTag, CooldownTagParam
	);

	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnCooldownStartedSignature OnCooldownStarted;

	/** Called when skill is activated */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
		FOnSkillActivatedSignature,
		FGameplayTag, SkillTag
	);

	UPROPERTY(BlueprintAssignable, Category = "Skill|Events")
	FOnSkillActivatedSignature OnSkillActivated;

	// ========================================
	// Helper Functions
	// ========================================

	/** Get Skill Icon from Data */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	UTexture2D* GetSkillIcon() const;

	/** Get Skill Name from Data */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	FText GetSkillName() const;

	/** Get Skill Description from Data */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	FText GetSkillDescription() const;

	/** Get Cooldown Duration from Data */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	float GetCooldownDuration() const;

	/** EffectsToTarget 배열에서 Index번째 GE 클래스 반환 (FireZone 등 존 기반 스킬용) */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	TSubclassOf<UGameplayEffect> GetTargetEffectClass(int32 Index = 0) const;
};