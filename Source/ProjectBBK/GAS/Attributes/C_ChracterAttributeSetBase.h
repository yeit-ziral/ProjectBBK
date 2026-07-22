// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "C_ChracterAttributeSetBase.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_ChracterAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, Category = "level", ReplicatedUsing = OnRep_level)
	FGameplayAttributeData level;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, level)

	UPROPERTY(BlueprintReadOnly, Category = "experience", ReplicatedUsing = OnRep_experience)
	FGameplayAttributeData experience;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, experience)

	UPROPERTY(BlueprintReadOnly, Category = "experience", ReplicatedUsing = OnRep_maxExperience)
	FGameplayAttributeData maxExperience;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, maxExperience)

	UPROPERTY(BlueprintReadOnly, Category = "health", ReplicatedUsing = OnRep_health)
	FGameplayAttributeData health;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, health)

	UPROPERTY(BlueprintReadOnly, Category = "health", ReplicatedUsing = OnRep_maxHealth)
	FGameplayAttributeData maxHealth;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, maxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "shield", ReplicatedUsing = OnRep_shield)
	FGameplayAttributeData shield;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, shield)

	UPROPERTY(BlueprintReadOnly, Category = "shield", ReplicatedUsing = OnRep_maxShield)
	FGameplayAttributeData maxShield;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, maxShield)

		//this is for damage calculation temporary use  So, not replicated
	UPROPERTY(BlueprintReadOnly, Category = "damage")
	FGameplayAttributeData damage;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, damage)

	// 방어력 — 장비(갑옷)로 부여받아 받는 데미지를 고정 차감. 영구 스탯이므로 복제됨.
	UPROPERTY(BlueprintReadOnly, Category = "defense", ReplicatedUsing = OnRep_defense)
	FGameplayAttributeData defense;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, defense)

	UPROPERTY(BlueprintReadOnly, Category = "mana", ReplicatedUsing = OnRep_mana)
	FGameplayAttributeData mana;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, mana)

	UPROPERTY(BlueprintReadOnly, Category = "mana", ReplicatedUsing = OnRep_maxMana)
	FGameplayAttributeData maxMana;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, maxMana)

	UPROPERTY(BlueprintReadOnly, Category = "moveSpeed", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData moveSpeed;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, moveSpeed)

	UPROPERTY(BlueprintReadOnly, Category = "stamina", ReplicatedUsing = OnRep_stamina)
	FGameplayAttributeData stamina;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, stamina)

	UPROPERTY(BlueprintReadOnly, Category = "stamina", ReplicatedUsing = OnRep_maxStamina)
	FGameplayAttributeData maxStamina;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, maxStamina)

	UPROPERTY(BlueprintReadOnly, Category = "ammo", ReplicatedUsing = OnRep_ammo)
	FGameplayAttributeData ammo;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, ammo)

	UPROPERTY(BlueprintReadOnly, Category = "ammo", ReplicatedUsing = OnRep_maxAmmo)
	FGameplayAttributeData maxAmmo;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, maxAmmo)

	UPROPERTY(BlueprintReadOnly, Category = "ammo", ReplicatedUsing = OnRep_reloadTime)
	FGameplayAttributeData reloadTime;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, reloadTime)

	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	FGameplayAttributeData receivedDamage;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, receivedDamage)
	
	// 방어력을 무시하는 고정 데미지 (DoT, 상태이상 전용)
	UPROPERTY(BlueprintReadOnly, Category = "Meta")
	FGameplayAttributeData receivedTrueDamage;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, receivedTrueDamage)

	UFUNCTION()
	virtual void OnRep_level(const FGameplayAttributeData& OldLevel);

	UFUNCTION()
	virtual void OnRep_experience(const FGameplayAttributeData& OldExperience);
	UFUNCTION()
	virtual void OnRep_maxExperience(const FGameplayAttributeData& OldMaxExperience);

	UFUNCTION()
	virtual void OnRep_health(const FGameplayAttributeData& OldHealth);
	UFUNCTION()
	virtual void OnRep_maxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_shield(const FGameplayAttributeData& OldShield);
	UFUNCTION()
	virtual void OnRep_maxShield(const FGameplayAttributeData& OldMaxShield);

	UFUNCTION()
	virtual void OnRep_mana(const FGameplayAttributeData& OldMana);

	UFUNCTION()
	virtual void OnRep_maxMana(const FGameplayAttributeData& OldMaxMana);

	UFUNCTION()
	virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);

	UFUNCTION()
	virtual void OnRep_stamina(const FGameplayAttributeData& OldStamina);

	UFUNCTION()
	virtual void OnRep_maxStamina(const FGameplayAttributeData& OldMaxStamina);

	UFUNCTION()
	virtual void OnRep_ReceivedDamage(const FGameplayAttributeData& OldReceivedDamage);

	UFUNCTION()
	virtual void OnRep_ReceivedTrueDamage(const FGameplayAttributeData& OldReceivedTrueDamage);

	UFUNCTION()
	virtual void OnRep_defense(const FGameplayAttributeData& OldDefense);

	UFUNCTION()
	virtual void OnRep_ammo(const FGameplayAttributeData& OldAmmo);

	UFUNCTION()
	virtual void OnRep_maxAmmo(const FGameplayAttributeData& OldMaxAmmo);

	UFUNCTION()
	virtual void OnRep_reloadTime(const FGameplayAttributeData& OldReloadTime);

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	void AdjustAttributeForMaxChange(
		FGameplayAttributeData& AffectedAttribute,
		const FGameplayAttributeData& MaxAttribute,
		float NewMaxValue,
		const FGameplayAttribute& AffectedAttributeProperty
	);
};
