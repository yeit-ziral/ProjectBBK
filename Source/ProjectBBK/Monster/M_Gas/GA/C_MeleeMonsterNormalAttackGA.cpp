// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MeleeMonsterNormalAttackGA.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "../../C_BaseMonster.h"

UC_MeleeMonsterNormalAttackGA::UC_MeleeMonsterNormalAttackGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_MeleeMonsterNormalAttackGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayAttackMontageAndBindEvents();
}

void UC_MeleeMonsterNormalAttackGA::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UC_MeleeMonsterNormalAttackGA::PlayAttackMontageAndBindEvents()
{
	if (!attackMontage)
	{
		// ��Ÿ�ְ� ������ �׳� ����
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
		return;
	}

	// 1) ��Ÿ�� ���
	UAbilityTask_PlayMontageAndWait* montageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy
	(
		this, NAME_None, attackMontage, 1.0f, NAME_None, false
	);

	if (montageTask)
	{
		montageTask->OnCompleted.AddDynamic(this, &UC_MeleeMonsterNormalAttackGA::OnMontageCompleted);
		montageTask->OnInterrupted.AddDynamic(this, &UC_MeleeMonsterNormalAttackGA::OnMontageCancelled);
		montageTask->OnCancelled.AddDynamic(this, &UC_MeleeMonsterNormalAttackGA::OnMontageCancelled);
		montageTask->ReadyForActivation();
	}

	// 노말 공격음 (MM_Monster_Arm_Swing_one_time) — 스윙 시작 시점
	if (attackSound)
	{
		if (AActor* avatar = GetAvatarActorFromActorInfo())
			UGameplayStatics::PlaySoundAtLocation(avatar, attackSound, avatar->GetActorLocation());
	}

	// 2) ��Ʈ Ÿ�̹� �̺�Ʈ ���
	if (hitEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* hitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, hitEventTag, nullptr, false, false);

		if (hitEventTask)
		{
			hitEventTask->EventReceived.AddDynamic(this, &UC_MeleeMonsterNormalAttackGA::OnHitEventReceived);
			hitEventTask->ReadyForActivation();
		}
	}
}

void UC_MeleeMonsterNormalAttackGA::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UC_MeleeMonsterNormalAttackGA::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UC_MeleeMonsterNormalAttackGA::OnHitEventReceived(FGameplayEventData Payload)
{
	AC_BaseMonster* monster = Cast<AC_BaseMonster>(GetAvatarActorFromActorInfo());
	if (!monster) return;

	const FVector start = monster->GetActorLocation();
	const FVector end   = start + monster->GetActorForwardVector() * monster->GetAttackRange();

	TArray<TEnumAsByte<EObjectTypeQuery>> types;
	types.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	TArray<AActor*> ignore;
	ignore.Add(monster);
	TArray<AActor*> allMonsters;
	UGameplayStatics::GetAllActorsOfClass(monster->GetWorld(), AC_BaseMonster::StaticClass(), allMonsters);
	for (AActor* m : allMonsters) ignore.Add(m);

	FHitResult hit;
	const bool hitOk = UKismetSystemLibrary::SphereTraceSingleForObjects(
		monster, start, end, traceRadius, types, false, ignore,
		EDrawDebugTrace::None, hit, true
	);

	if (hitOk && hit.GetActor())
		ApplyDamageToTarget(hit.GetActor());
}

void UC_MeleeMonsterNormalAttackGA::ApplyDamageToTarget(AActor* TargetActor)
{
	if (!damageEffectClass || !TargetActor) return;

	UAbilitySystemComponent* sourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!sourceASC) return;

	UAbilitySystemComponent* targetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!targetASC) return;

	// DataTable Attack 스탯 사용
	AC_BaseMonster* monster = Cast<AC_BaseMonster>(GetAvatarActorFromActorInfo());
	const float attackValue = monster ? (float)monster->GetAttack() * damageMultiplier : 0.f;

	FGameplayEffectContextHandle effectContext = sourceASC->MakeEffectContext();
	effectContext.AddInstigator(CurrentActorInfo->AvatarActor.Get(), CurrentActorInfo->OwnerActor.Get());

	FGameplayEffectSpecHandle specHandle = sourceASC->MakeOutgoingSpec(damageEffectClass, GetAbilityLevel(), effectContext);
	if (!specHandle.IsValid()) return;

	if (setByCallerDamageTag.IsValid())
		specHandle.Data->SetSetByCallerMagnitude(setByCallerDamageTag, attackValue);

	sourceASC->ApplyGameplayEffectSpecToTarget(*specHandle.Data.Get(), targetASC);
}
