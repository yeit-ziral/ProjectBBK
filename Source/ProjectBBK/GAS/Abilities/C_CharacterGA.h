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

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Ability")
	ProjectBBKAbilityID abilityInputID = ProjectBBKAbilityID::None; // To bind ability to input so this will active whenever input is pressed

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Ability")
	ProjectBBKAbilityID abilityID = ProjectBBKAbilityID::None; // To identify ability so this will be used for something regardless of input key like passive ability

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability")
	bool activateOnGranted = false; // if true, ability will activate as soon as it's granted to ability system component
};
