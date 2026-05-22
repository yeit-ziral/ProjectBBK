// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BossBeamPatternGA.h"
#include "../../Object/C_BossBeam.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UC_BossBeamPatternGA::UC_BossBeamPatternGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_BossBeamPatternGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* boss = GetAvatarActorFromActorInfo();
	if (!boss || !beamClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ACharacter* bossChar = Cast<ACharacter>(boss);

	// 빔 패턴 중 일반 공격 차단
	if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo())
		asc->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Boss.BeamPattern")));

	// 차징 몽타주를 이동 정지보다 먼저 재생
	// DisableMovement()는 MOVE_None으로 전환해 AnimBP 슬롯을 꺼버릴 수 있으므로
	// MaxWalkSpeed=0으로 이동만 막고 MovementMode는 Walking 유지
	if (chargingMontage && bossChar)
	{
		float montageLen = chargingMontage->GetPlayLength();
		float rate = (montageLen > 0.f && chargingDuration > 0.f) ? montageLen / chargingDuration : 1.f;
		bossChar->PlayAnimMontage(chargingMontage, rate);
	}

	if (bossChar)
	{
		savedMaxWalkSpeed = bossChar->GetCharacterMovement()->MaxWalkSpeed;
		bossChar->GetCharacterMovement()->StopMovementImmediately();
		bossChar->GetCharacterMovement()->MaxWalkSpeed = 0.f;
	}

	// 빔 즉시 스폰 — BeginPlay에서 마법진(previewPlane) 자동 표시
	// GA가 타이밍을 직접 제어하므로 빔 내부 자동 전환 타이머는 비활성화
	float angleStep = 360.f / beamCount;
	spawnedBeams.Empty();
	for (int32 i = 0; i < beamCount; ++i)
	{
		FRotator rot(0.f, angleStep * i, 0.f);
		AC_BossBeam* beam = GetWorld()->SpawnActor<AC_BossBeam>(beamClass, boss->GetActorLocation(), rot);
		if (beam)
		{
			beam->ownerBoss = boss;
			beam->SetLifeSpan(0.f);
			beam->DisableAutoSwitch();
			spawnedBeams.Add(beam);
		}
	}

	// chargingDuration 후 발사 전환
	GetWorld()->GetTimerManager().SetTimer(
		chargingTimerHandle,
		this,
		&UC_BossBeamPatternGA::StartFiringPhase,
		chargingDuration,
		false
	);
}

void UC_BossBeamPatternGA::StartFiringPhase()
{
	AActor* boss = GetAvatarActorFromActorInfo();
	if (!boss) return;

	// 빔 발사 전환
	for (auto& weakBeam : spawnedBeams)
	{
		if (weakBeam.IsValid())
			weakBeam->SwitchToBeam();
	}

	// 발사 몽타주 재생
	if (ACharacter* bossChar = Cast<ACharacter>(boss))
	{
		if (firingMontage)
			bossChar->PlayAnimMontage(firingMontage);
	}

	// beamDuration 후 GA 종료
	GetWorld()->GetTimerManager().SetTimer(
		beamDurationTimerHandle,
		this,
		&UC_BossBeamPatternGA::OnBeamDurationExpired,
		beamDuration,
		false
	);
}

void UC_BossBeamPatternGA::OnBeamDurationExpired()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UC_BossBeamPatternGA::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AActor* boss = GetAvatarActorFromActorInfo())
	{
		if (ACharacter* bossChar = Cast<ACharacter>(boss))
		{
			bossChar->GetCharacterMovement()->MaxWalkSpeed = savedMaxWalkSpeed;

			if (chargingMontage) bossChar->StopAnimMontage(chargingMontage);
			if (firingMontage)   bossChar->StopAnimMontage(firingMontage);
		}
	}

	if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo())
		asc->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Boss.BeamPattern")));

	for (auto& weakBeam : spawnedBeams)
	{
		if (weakBeam.IsValid())
			weakBeam->Destroy();
	}
	spawnedBeams.Empty();

	if (UWorld* world = GetWorld())
	{
		world->GetTimerManager().ClearTimer(chargingTimerHandle);
		world->GetTimerManager().ClearTimer(beamDurationTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
