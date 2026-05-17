// Fill out your copyright notice in the Description page of Project Settings.

#include "C_RangedUltimate.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameplayEffect.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "../Monster/C_BaseMonster.h"

UC_RangedUltimate::UC_RangedUltimate()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NotifyEventTag   = FGameplayTag::RequestGameplayTag(FName("Event.Ultimate.Fire"));
}

void UC_RangedUltimate::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 몽타주 재생 태스크
	FSkillData SkillData;
	UAnimMontage* Montage = GetSkillData(SkillData) ? SkillData.castAnimation : nullptr;

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, Montage);

	MontageTask->OnCompleted.AddDynamic(this, &UC_RangedUltimate::OnMontageEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &UC_RangedUltimate::OnMontageEnded);
	MontageTask->OnCancelled.AddDynamic(this, &UC_RangedUltimate::OnMontageEnded);

	// Notify 수신 태스크
	UAbilityTask_WaitGameplayEvent* EventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, NotifyEventTag);

	EventTask->EventReceived.AddDynamic(this, &UC_RangedUltimate::HandleNotifyEvent);

	MontageTask->ReadyForActivation();
	EventTask->ReadyForActivation();

	// 이동 입력 차단 (카메라 입력은 유지)
	if (APlayerController* PC = Cast<APlayerController>(ActorInfo->PlayerController.Get()))
	{
		PC->SetIgnoreMoveInput(true);
	}
}

void UC_RangedUltimate::HandleNotifyEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	// Box 중심: 캐릭터 중심에서 전방으로 오프셋
	const FVector BoxCenter = AvatarActor->GetActorLocation()
		+ AvatarActor->GetActorForwardVector() * BoxOriginOffsetX;

	const FVector BoxExtent(BoxHalfExtentX, BoxHalfExtentY, BoxHalfExtentZ);

	// Box Overlap — Pawn 채널, AC_BaseMonster 필터
	TArray<AActor*> HitActors;
	UKismetSystemLibrary::BoxOverlapActors(
		GetWorld(),
		BoxCenter,
		BoxExtent,
		TArray<TEnumAsByte<EObjectTypeQuery>>{ ObjectTypeQuery3 },
		AC_BaseMonster::StaticClass(),
		TArray<AActor*>{ AvatarActor },
		HitActors);

	// 넉백 적용 — 방향: 시전자 → 각 타겟 (수평만), LaunchCharacter 직접 호출
	for (AActor* Target : HitActors)
	{
		if (!Target)
		{
			continue;
		}

		ACharacter* TargetCharacter = Cast<ACharacter>(Target);
		if (!TargetCharacter)
		{
			continue;
		}

		// 넉백 면역 태그 체크
		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (TargetASC && TargetASC->HasMatchingGameplayTag(
			FGameplayTag::RequestGameplayTag(FName("State.KnockbackImmune"))))
		{
			continue;
		}

		FVector Direction = Target->GetActorLocation() - AvatarActor->GetActorLocation();
		Direction.Z = 0.f;
		Direction.Normalize();

		TargetCharacter->LaunchCharacter(Direction * KnockbackForce, true, false);
	}

	// 데미지 GE + DrawDebugBox → Blueprint 처리
	OnNotifyReceived_BP(HitActors, BoxCenter, BoxExtent);
}

void UC_RangedUltimate::OnMontageEnded()
{
	const FGameplayAbilitySpecHandle Handle        = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo     = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActInfo   = GetCurrentActivationInfo();
	EndAbility(Handle, ActorInfo, ActInfo, true, false);
}

void UC_RangedUltimate::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 이동 입력 복구
	if (ActorInfo)
	{
		if (APlayerController* PC = Cast<APlayerController>(ActorInfo->PlayerController.Get()))
		{
			PC->ResetIgnoreMoveInput();
		}
	}

	ApplySkillCooldown();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
