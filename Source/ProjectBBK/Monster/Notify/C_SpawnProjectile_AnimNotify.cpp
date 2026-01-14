// Fill out your copyright notice in the Description page of Project Settings.


#include "C_SpawnProjectile_AnimNotify.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"

void UC_SpawnProjectile_AnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp)
		return;

	AActor* ownerActor = MeshComp->GetOwner();
	if (!ownerActor)
		return;

	if (!FireEventTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FireProjectile Notify: EventTag invalid"));
		return;
	}

	FGameplayEventData payload;
	payload.Instigator = ownerActor;
	payload.Target = nullptr;

	// GA로 이벤트 전송
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ownerActor, FireEventTag, payload);
}
