// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "../GAS/Attributes/C_ChracterAttributeSetBase.h"
#include "../GAS/Abilities/C_CharacterASC.h"
#include "GameplayEffectTypes.h"
#include "C_PlayerState.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API AC_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AC_PlayerState();
	
	class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	class UC_ChracterAttributeSetBase* GetAttributeSetBase() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|PlayerCharacterState")
	bool IsAlive() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|PlayerCharacterState|UI")
	void ShowAbilityConfirmCancelText(bool ShowText);

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|PlayerCharacterState|Attributes")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|PlayerCharacterState|Attributes")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|PlayerCharacterState|Attributes")
	float GetShield() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|PlayerCharacterState|Attributes")
	float GetMaxShield() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|PlayerCharacterState|Attributes")
	int32 GetCharacterLevel() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|PlayerCharacterState|Attributes")
	float GetStamina() const;

	UFUNCTION(BlueprintCallable, Category = "ProjectBBK|PlayerCharacterState|Attributes")
	float GetMaxStamina() const;

protected:
	virtual void BeginPlay() override;

	virtual void HealthChanged(const FOnAttributeChangeData& Data);
	virtual void MaxHealthChanged(const FOnAttributeChangeData& Data);
	
	virtual void ShieldChanged(const FOnAttributeChangeData& Data);
	virtual void MaxShieldChanged(const FOnAttributeChangeData& Data);

	virtual void StaminaChanged(const FOnAttributeChangeData& Data);
	virtual void MaxStaminaChanged(const FOnAttributeChangeData& Data);
	
	virtual void CharacterLevelChanged(const FOnAttributeChangeData& Data);

	virtual void StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

protected:
	UPROPERTY()
	class UC_CharacterASC* abilitySystemComponent;

	UPROPERTY()
	class UC_ChracterAttributeSetBase* attributeSetBase;

	FGameplayTag deadTag;

	FDelegateHandle healthChangedDelegateHandle;
	FDelegateHandle maxHealthChangedDelegateHandle;

	FDelegateHandle shieldChangedDelegateHandle;
	FDelegateHandle maxShieldChangedDelegateHandle;

	FDelegateHandle characterLevelChangedDelegateHandle;

	FDelegateHandle staminaChangedDelegateHandle;
	FDelegateHandle maxStaminaChangedDelegateHandle;
};
