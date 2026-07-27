// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EliteMonsterSpecialAttackGA_Pull.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "../../C_BaseMonster.h"
#include "../../Object/C_EliteBlackSphere.h"

UC_EliteMonsterSpecialAttackGA_Pull::UC_EliteMonsterSpecialAttackGA_Pull()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_EliteMonsterSpecialAttackGA_Pull::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	hitEventTag     = FGameplayTag::RequestGameplayTag(TEXT("Event.Monster.Elite.SphereHit"));
	throwEventTag   = FGameplayTag::RequestGameplayTag(TEXT("Event.Monster.Elite.ThrowRelease"));
	pullActivateTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Monster.Elite.PullActivate"));
	bSphereSpawned = false;
	bDragStarted   = false;

	AActor* avatar = GetAvatarActorFromActorInfo();
	UWorld* world  = avatar ? avatar->GetWorld() : nullptr;
	if (!avatar || !world)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 시전 시 플레이어를 바라보도록 Yaw 정렬 (몬스터는 bUseControllerRotationYaw=true)
	if (APawn* player = UGameplayStatics::GetPlayerPawn(world, 0))
	{
		FVector toPlayer = player->GetActorLocation() - avatar->GetActorLocation();
		toPlayer.Z = 0.f;
		if (!toPlayer.IsNearlyZero())
		{
			FRotator look = toPlayer.Rotation();
			look.Pitch = 0.f; look.Roll = 0.f;
			avatar->SetActorRotation(look);
		}
	}

	// 던지기 애니 재생 (연출)
	PlayMonsterAnim(throwAnim);

	// 던지기 타이밍 AnimNotify 이벤트 → 구체 발사
	if (throwEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* throwTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, throwEventTag, nullptr, false, false);
		if (throwTask)
		{
			throwTask->EventReceived.AddDynamic(this, &UC_EliteMonsterSpecialAttackGA_Pull::OnThrowRelease);
			throwTask->ReadyForActivation();
		}
	}

	// 명중 이벤트 대기 → 끌어오기
	if (hitEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* hitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, hitEventTag, nullptr, false, false);
		if (hitTask)
		{
			hitTask->EventReceived.AddDynamic(this, &UC_EliteMonsterSpecialAttackGA_Pull::OnSphereHit);
			hitTask->ReadyForActivation();
		}
	}

	// 노티파이 미배치 대비 안전 폴백 (0이면 폴백 없이 노티파이만 사용)
	if (sphereSpawnFallbackDelay > 0.f)
	{
		world->GetTimerManager().SetTimer(
			spawnTimerHandle, this, &UC_EliteMonsterSpecialAttackGA_Pull::SpawnSphere, sphereSpawnFallbackDelay, false);
	}

	// 명중 못 하면 자동 종료
	world->GetTimerManager().SetTimer(
		timeoutTimerHandle, [this]() { EndNow(false); }, FMath::Max(0.1f, maxWaitForHit), false);
}

void UC_EliteMonsterSpecialAttackGA_Pull::OnThrowRelease(FGameplayEventData Payload)
{
	// 던지는 프레임 노티파이 → 즉시 발사 (폴백 타이머는 무효화)
	if (UWorld* world = GetWorld())
		world->GetTimerManager().ClearTimer(spawnTimerHandle);
	SpawnSphere();
}

void UC_EliteMonsterSpecialAttackGA_Pull::SpawnSphere()
{
	if (bSphereSpawned || !sphereClass) return;

	AActor* avatar = GetAvatarActorFromActorInfo();
	UWorld* world  = avatar ? avatar->GetWorld() : nullptr;
	if (!avatar || !world) return;

	APawn* player = UGameplayStatics::GetPlayerPawn(world, 0);
	if (!player) return;

	const FVector fwd   = avatar->GetActorForwardVector();
	const FVector right = avatar->GetActorRightVector();
	const FVector up    = avatar->GetActorUpVector();
	const FVector spawnLoc = avatar->GetActorLocation()
		+ fwd   * sphereSpawnOffset.X
		+ right * sphereSpawnOffset.Y
		+ up    * sphereSpawnOffset.Z;

	// 플레이어 중심(캡슐 중앙 근처)을 겨냥
	const FVector aimPoint = player->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const FVector dir = (aimPoint - spawnLoc).GetSafeNormal();

	FActorSpawnParameters params;
	params.Owner = avatar;
	params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AC_EliteBlackSphere* sphere = world->SpawnActor<AC_EliteBlackSphere>(
		sphereClass, spawnLoc, dir.Rotation(), params);
	if (!sphere) return;

	// DataTable Attack 스탯 기반 데미지
	AC_BaseMonster* monster = Cast<AC_BaseMonster>(avatar);
	const float damage = monster ? (float)monster->GetAttack() * damageMultiplier : 0.f;

	sphere->InitSphere(GetAbilitySystemComponentFromActorInfo(), avatar, damageEffectClass, damage, dir, hitEventTag);
	bSphereSpawned = true;

	// 검은 구체 발사음 (EM_BlackOrbLaunch)
	if (launchSound)
		UGameplayStatics::PlaySoundAtLocation(avatar, launchSound, spawnLoc);
}

void UC_EliteMonsterSpecialAttackGA_Pull::OnSphereHit(FGameplayEventData Payload)
{
	UWorld* world = GetWorld();
	if (!world) { EndNow(false); return; }

	// 명중 대상(플레이어) 확보 — 페이로드 우선, 없으면 0번 플레이어
	ACharacter* player = Cast<ACharacter>(const_cast<AActor*>(Payload.Target.Get()));
	if (!player)
		player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(world, 0));
	pulledPlayer = player;

	world->GetTimerManager().ClearTimer(timeoutTimerHandle);

	// pullDelay(약 1초) 뒤 끌어오기 시작
	world->GetTimerManager().SetTimer(
		pullDelayTimerHandle, this, &UC_EliteMonsterSpecialAttackGA_Pull::StartPull, FMath::Max(0.01f, pullDelay), false);
}

void UC_EliteMonsterSpecialAttackGA_Pull::StartPull()
{
	AActor* avatar = GetAvatarActorFromActorInfo();
	UWorld* world  = avatar ? avatar->GetWorld() : nullptr;
	if (!avatar || !world || !pulledPlayer.IsValid())
	{
		EndNow(false);
		return;
	}

	// 끌어오기 애니 재생 — 실제 드래그는 애니의 '당기는' 프레임 노티파이가 트리거
	PlayMonsterAnim(pullAnim);
	bDragStarted = false;

	if (pullActivateTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* pullTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, pullActivateTag, nullptr, false, false);
		if (pullTask)
		{
			pullTask->EventReceived.AddDynamic(this, &UC_EliteMonsterSpecialAttackGA_Pull::OnPullActivate);
			pullTask->ReadyForActivation();
		}
	}

	// 노티파이 미배치 대비 폴백 (0이면 노티파이만)
	if (pullActivateFallbackDelay > 0.f)
	{
		world->GetTimerManager().SetTimer(
			pullActivateTimerHandle, [this]() { BeginDrag(); }, pullActivateFallbackDelay, false);
	}
}

void UC_EliteMonsterSpecialAttackGA_Pull::OnPullActivate(FGameplayEventData Payload)
{
	// 당기는 프레임 노티파이 → 즉시 드래그 (폴백 무효화)
	if (UWorld* world = GetWorld())
		world->GetTimerManager().ClearTimer(pullActivateTimerHandle);
	BeginDrag();
}

void UC_EliteMonsterSpecialAttackGA_Pull::BeginDrag()
{
	if (bDragStarted) return;

	AActor* avatar = GetAvatarActorFromActorInfo();
	UWorld* world  = avatar ? avatar->GetWorld() : nullptr;
	if (!avatar || !world || !pulledPlayer.IsValid())
	{
		EndNow(false);
		return;
	}
	bDragStarted = true;

	pullStart = pulledPlayer->GetActorLocation();

	FVector dirH = pullStart - avatar->GetActorLocation();
	dirH.Z = 0.f;
	dirH = dirH.GetSafeNormal();
	if (dirH.IsNearlyZero())
		dirH = avatar->GetActorForwardVector();

	pullTarget = avatar->GetActorLocation() + dirH * pullStopDistance;
	pullTarget.Z = pullStart.Z;   // 수평 끌어오기 (수직 yank 방지)

	pullElapsed = 0.f;
	SetPlayerMoveInputBlocked(true);

	// 적을 끌어오기 시작하는 순간의 그랩음 (EM_Grab)
	if (grabSound)
		UGameplayStatics::PlaySoundAtLocation(avatar, grabSound, avatar->GetActorLocation());

	world->GetTimerManager().SetTimer(
		pullTickTimerHandle, this, &UC_EliteMonsterSpecialAttackGA_Pull::TickPull, pullTickInterval, true);
}

void UC_EliteMonsterSpecialAttackGA_Pull::TickPull()
{
	if (!pulledPlayer.IsValid())
	{
		FinishPull();
		return;
	}

	pullElapsed += pullTickInterval;
	const float alpha = (pullDuration > 0.f) ? FMath::Clamp(pullElapsed / pullDuration, 0.f, 1.f) : 1.f;
	const float eased = alpha * alpha * (3.f - 2.f * alpha);   // smoothstep

	const FVector newLoc = FMath::Lerp(pullStart, pullTarget, eased);
	pulledPlayer->SetActorLocation(newLoc, true);   // sweep → 벽 통과 방지

	if (alpha >= 1.f)
		FinishPull();
}

void UC_EliteMonsterSpecialAttackGA_Pull::FinishPull()
{
	if (UWorld* world = GetWorld())
		world->GetTimerManager().ClearTimer(pullTickTimerHandle);

	SetPlayerMoveInputBlocked(false);
	EndNow(false);
}

void UC_EliteMonsterSpecialAttackGA_Pull::PlayMonsterAnim(UAnimSequenceBase* Anim)
{
	if (!Anim) return;

	ACharacter* monster = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!monster) return;

	USkeletalMeshComponent* mesh = monster->GetMesh();
	UAnimInstance* anim = mesh ? mesh->GetAnimInstance() : nullptr;
	if (anim)
		anim->PlaySlotAnimationAsDynamicMontage(Anim, animSlotName, 0.2f, 0.2f, 1.0f, 1);
}

void UC_EliteMonsterSpecialAttackGA_Pull::SetPlayerMoveInputBlocked(bool bBlocked)
{
	if (bInputBlocked == bBlocked) return;
	if (!pulledPlayer.IsValid()) { bInputBlocked = false; return; }

	if (APlayerController* pc = Cast<APlayerController>(pulledPlayer->GetController()))
	{
		if (bBlocked) pc->SetIgnoreMoveInput(true);
		else          pc->SetIgnoreMoveInput(false);
		bInputBlocked = bBlocked;
	}
}

void UC_EliteMonsterSpecialAttackGA_Pull::EndNow(bool bCancelled)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
}

void UC_EliteMonsterSpecialAttackGA_Pull::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* world = GetWorld())
	{
		FTimerManager& tm = world->GetTimerManager();
		tm.ClearTimer(spawnTimerHandle);
		tm.ClearTimer(pullDelayTimerHandle);
		tm.ClearTimer(pullActivateTimerHandle);
		tm.ClearTimer(pullTickTimerHandle);
		tm.ClearTimer(timeoutTimerHandle);
	}

	// 안전: 끌어오기 중 취소돼도 플레이어 입력 복구
	SetPlayerMoveInputBlocked(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
