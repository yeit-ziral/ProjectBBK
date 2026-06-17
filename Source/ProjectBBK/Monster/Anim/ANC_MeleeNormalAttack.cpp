// Fill out your copyright notice in the Description page of Project Settings.

#include "ANC_MeleeNormalAttack.h"
#include "../C_BaseMonster.h"
#include "AbilitySystemBlueprintLibrary.h"

void UANC_MeleeNormalAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;

	AC_BaseMonster* monster = Cast<AC_BaseMonster>(MeshComp->GetOwner());
	if (!monster) return;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		monster,
		FGameplayTag::RequestGameplayTag(TEXT("Event.Monster.Melee.Hit")),
		FGameplayEventData()
	);
}
