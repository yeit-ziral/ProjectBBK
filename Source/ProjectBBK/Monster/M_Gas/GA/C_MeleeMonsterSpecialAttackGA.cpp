// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MeleeMonsterSpecialAttackGA.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "../../C_BaseMonster.h"

UC_MeleeMonsterSpecialAttackGA::UC_MeleeMonsterSpecialAttackGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_MeleeMonsterSpecialAttackGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!attackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* montageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, attackMontage, 1.0f, NAME_None, false
	);
	if (montageTask)
	{
		montageTask->OnCompleted.AddDynamic(this, &UC_MeleeMonsterSpecialAttackGA::OnMontageCompleted);
		montageTask->OnInterrupted.AddDynamic(this, &UC_MeleeMonsterSpecialAttackGA::OnMontageCancelled);
		montageTask->OnCancelled.AddDynamic(this, &UC_MeleeMonsterSpecialAttackGA::OnMontageCancelled);
		montageTask->ReadyForActivation();
	}

	if (hitEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* hitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, hitEventTag, nullptr, false, false
		);
		if (hitEventTask)
		{
			hitEventTask->EventReceived.AddDynamic(this, &UC_MeleeMonsterSpecialAttackGA::OnHitEventReceived);
			hitEventTask->ReadyForActivation();
		}
	}
}

void UC_MeleeMonsterSpecialAttackGA::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UC_MeleeMonsterSpecialAttackGA::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UC_MeleeMonsterSpecialAttackGA::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UC_MeleeMonsterSpecialAttackGA::OnHitEventReceived(FGameplayEventData Payload)
{
	ApplySlamDamageAndKnockback();
}

void UC_MeleeMonsterSpecialAttackGA::ApplySlamDamageAndKnockback()
{
	if (!damageEffectClass) return;

	AC_BaseMonster* monster = Cast<AC_BaseMonster>(GetAvatarActorFromActorInfo());
	if (!monster) return;

	UAbilitySystemComponent* sourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!sourceASC) return;

	const FVector center = monster->GetActorLocation();

	// 땅을 내려찍는 순간의 슬램음 (MM_Monster_Ground_Slam)
	if (slamSound)
		UGameplayStatics::PlaySoundAtLocation(monster, slamSound, center);
	const float attackValue = (float)monster->GetAttack() * slamDamageMultiplier;

	TArray<TEnumAsByte<EObjectTypeQuery>> types;
	types.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	TArray<AActor*> ignore;
	ignore.Add(monster);
	TArray<AActor*> allMonsters;
	UGameplayStatics::GetAllActorsOfClass(monster->GetWorld(), AC_BaseMonster::StaticClass(), allMonsters);
	for (AActor* m : allMonsters) ignore.Add(m);

	TArray<AActor*> hitActors;
	UKismetSystemLibrary::SphereOverlapActors(monster, center, slamRadius, types, nullptr, ignore, hitActors);

	for (AActor* hitActor : hitActors)
	{
		UAbilitySystemComponent* targetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(hitActor);
		if (!targetASC) continue;

		FGameplayEffectContextHandle effectContext = sourceASC->MakeEffectContext();
		effectContext.AddInstigator(CurrentActorInfo->AvatarActor.Get(), CurrentActorInfo->OwnerActor.Get());

		FGameplayEffectSpecHandle specHandle = sourceASC->MakeOutgoingSpec(damageEffectClass, GetAbilityLevel(), effectContext);
		if (!specHandle.IsValid()) continue;

		if (setByCallerDamageTag.IsValid())
			specHandle.Data->SetSetByCallerMagnitude(setByCallerDamageTag, attackValue);

		sourceASC->ApplyGameplayEffectSpecToTarget(*specHandle.Data.Get(), targetASC);

		// 넉백: 수평(뒤로 밀리는)·수직(위로 뜨는) 독립 적용
		if (ACharacter* targetChar = Cast<ACharacter>(hitActor))
		{
			const FVector horizontal = (hitActor->GetActorLocation() - center).GetSafeNormal2D() * knockbackStrength;
			const FVector vertical   = FVector(0.f, 0.f, verticalKnockbackStrength);
			targetChar->LaunchCharacter(horizontal + vertical, true, true);
		}
	}
}
