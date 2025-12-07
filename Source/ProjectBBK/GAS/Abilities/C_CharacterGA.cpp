// Fill out your copyright notice in the Description page of Project Settings.


#include "C_CharacterGA.h"
#include "C_CharacterASC.h"

UC_CharacterGA::UC_CharacterGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Debuff.Stun")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Input.Attack"));
}

void UC_CharacterGA::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (activateOnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
	}
}