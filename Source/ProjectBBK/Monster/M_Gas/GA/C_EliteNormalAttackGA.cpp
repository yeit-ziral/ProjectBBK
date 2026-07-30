// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EliteNormalAttackGA.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "../../C_BaseMonster.h"

UC_EliteNormalAttackGA::UC_EliteNormalAttackGA()
{
	// 교대 플래그를 활성화 간 유지하려면 액터당 단일 인스턴스 필요
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_EliteNormalAttackGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 두 스윙 몽타주를 교대 선택. 선택된 쪽이 비어 있으면 다른 쪽으로 폴백.
	UAnimMontage* chosen = bUseSecondMontage ? attackMontage2 : attackMontage1;
	if (!chosen)
		chosen = bUseSecondMontage ? attackMontage1 : attackMontage2;
	bUseSecondMontage = !bUseSecondMontage;

	if (!chosen)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// 노말 공격음 (EM_Grim_Hammer_Lord) — 스윙 시작 시점
	if (attackSound)
	{
		if (AActor* avatar = GetAvatarActorFromActorInfo())
			UGameplayStatics::PlaySoundAtLocation(avatar, attackSound, avatar->GetActorLocation());
	}

	// 1) 몽타주 재생 (Return2Idle 포함 → 완료 시 공격 종료)
	UAbilityTask_PlayMontageAndWait* montageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, chosen, 1.0f, NAME_None, false
	);
	if (montageTask)
	{
		montageTask->OnCompleted.AddDynamic(this, &UC_EliteNormalAttackGA::OnMontageCompleted);
		montageTask->OnInterrupted.AddDynamic(this, &UC_EliteNormalAttackGA::OnMontageCancelled);
		montageTask->OnCancelled.AddDynamic(this, &UC_EliteNormalAttackGA::OnMontageCancelled);
		montageTask->ReadyForActivation();
	}

	// 2) hit 타이밍 이벤트 대기
	if (hitEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* hitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, hitEventTag, nullptr, false, false
		);
		if (hitEventTask)
		{
			hitEventTask->EventReceived.AddDynamic(this, &UC_EliteNormalAttackGA::OnHitEventReceived);
			hitEventTask->ReadyForActivation();
		}
	}
}

void UC_EliteNormalAttackGA::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UC_EliteNormalAttackGA::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UC_EliteNormalAttackGA::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UC_EliteNormalAttackGA::OnHitEventReceived(FGameplayEventData Payload)
{
	AC_BaseMonster* monster = Cast<AC_BaseMonster>(GetAvatarActorFromActorInfo());
	if (!monster) return;

	const FVector start = monster->GetActorLocation();
	const FVector end   = start + monster->GetActorForwardVector() * monster->GetAttackRange();

	TArray<TEnumAsByte<EObjectTypeQuery>> types;
	types.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	// 자기 자신 + 다른 몬스터 전부 무시 (플레이어만 대상)
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

void UC_EliteNormalAttackGA::ApplyDamageToTarget(AActor* TargetActor)
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
