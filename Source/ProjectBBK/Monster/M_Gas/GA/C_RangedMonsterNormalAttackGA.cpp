// Fill out your copyright notice in the Description page of Project Settings.


#include "C_RangedMonsterNormalAttackGA.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"               
#include "Kismet/GameplayStatics.h" 

UC_RangedMonsterNormalAttackGA::UC_RangedMonsterNormalAttackGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_RangedMonsterNormalAttackGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ACharacter* character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!character || !AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AActor* targetActor = nullptr;

	if (AAIController* aiController = Cast<AAIController>(character->GetController()))
	{
		targetActor = aiController->GetFocusActor(); 
	}

	if (!targetActor)
	{
		targetActor = UGameplayStatics::GetPlayerPawn(character->GetWorld(), 0);
	}

	if (targetActor)
	{
		const FVector toTarget = (targetActor->GetActorLocation() - character->GetActorLocation());
		const FRotator lookRot = toTarget.Rotation();
		const FRotator newRot(0.f, lookRot.Yaw, 0.f);
		character->SetActorRotation(newRot);
	}

	character->PlayAnimMontage(AttackMontage);

	// 지금은 “모션만”
	// 나중에:
	// - AnimNotify에서 Projectile Spawn
	// - 또는 WaitMontageNotify / WaitMontageEnd 추가

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
