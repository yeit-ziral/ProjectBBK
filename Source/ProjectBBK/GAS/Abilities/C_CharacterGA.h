// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "../../ProjectBBK.h"
#include "C_CharacterGA.generated.h"

/**
 *
 */
UCLASS()
class PROJECTBBK_API UC_CharacterGA : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UC_CharacterGA();

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) override;

	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	void OnInputPressed();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Ability")
	ProjectBBKAbilityID abilityInputID = ProjectBBKAbilityID::None; // To bind ability to input so this will active whenever input is pressed

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability")
	bool activateOnGranted = false; // if true, ability will activate as soon as it's granted to ability system component

#pragma region skill
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Info")
	FText SkillName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Info")
	FText SkillDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Info")
	UTexture2D *SkillIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Info")
	int32 SkillLevel = 1;

protected:
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
#pragma endregion
};
