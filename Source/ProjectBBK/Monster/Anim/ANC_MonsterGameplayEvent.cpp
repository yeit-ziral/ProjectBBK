// Fill out your copyright notice in the Description page of Project Settings.

#include "ANC_MonsterGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"

void UANC_MonsterGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp || !eventTag.IsValid()) return;

	AActor* owner = MeshComp->GetOwner();
	if (!owner) return;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(owner, eventTag, FGameplayEventData());
}
