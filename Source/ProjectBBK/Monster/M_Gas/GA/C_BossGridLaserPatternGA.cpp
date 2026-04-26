// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossGridLaserPatternGA.h"
#include "../../Object/C_BossGhostClone.h"
#include "../../Object/C_GridLaser.h"
#include "../C_MonsterASC.h"
#include "../../../PlayerCharacter/C_BasePlayerCharactor.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

UC_BossGridLaserPatternGA::UC_BossGridLaserPatternGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_BossGridLaserPatternGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	cachedBossActor = GetAvatarActorFromActorInfo();
	if (!cachedBossActor || !ghostCloneClass || !gridLaserClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const FGridLaserPatternRow* row = gridPatternRow.GetRow<FGridLaserPatternRow>(TEXT("UC_BossGridLaserPatternGA"));
	if (!row)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GridLaserGA] DT row not found. Check gridPatternRow binding."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	cachedRow = *row;

	cachedMonsterASC = Cast<UC_MonsterASC>(GetAbilitySystemComponentFromActorInfo());
	if (cachedMonsterASC)
		cachedMonsterASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("State.Invincible"));

	// 카메라 탑다운 전환
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AC_BasePlayerCharactor* player = Cast<AC_BasePlayerCharactor>(PC->GetPawn()))
		{
			playerRef = player;
			player->SetTopDownCamera(true, cachedRow.cameraBlendTime,
				cachedRow.cameraTopDownArm, cachedRow.cameraTopDownPitch);
		}
	}

	const FVector center = playerRef.IsValid()
		? playerRef->GetActorLocation()
		: cachedBossActor->GetActorLocation();

	TArray<FVector> hPositions, vPositions;
	ComputeGridPositions(center, hPositions, vPositions);
	SpawnClones(hPositions, vPositions);
	PickMarkedClones();

	UE_LOG(LogTemp, Warning, TEXT("[GridLaserGA] Activate. Center=%s"), *center.ToString());

	GetWorld()->GetTimerManager().SetTimer(
		chargeTimerHandle, this, &UC_BossGridLaserPatternGA::OnChargeFinished,
		cachedRow.chargeDuration, false);
}

void UC_BossGridLaserPatternGA::ComputeGridPositions(const FVector& Center,
	TArray<FVector>& OutH, TArray<FVector>& OutV)
{
	const int32 N = FMath::Max(1, cachedRow.lineCount);
	const float halfSpan = (N - 1) * 0.5f * cachedRow.gridSpacing;
	UWorld* world = GetWorld();
	if (!world) return;

	// 가로 클론: 왼쪽 바깥에 배치, 오른쪽(+X)을 바라봄, Y축 슬라이스
	for (int32 i = 0; i < N; ++i)
	{
		const float y = Center.Y - halfSpan + cachedRow.gridSpacing * i;
		const float x = Center.X - halfSpan - cachedRow.cloneOffset;
		FVector flat(x, y, Center.Z);

		FHitResult hit;
		if (world->LineTraceSingleByChannel(hit, flat + FVector(0, 0, 800), flat - FVector(0, 0, 2000), ECC_WorldStatic))
			OutH.Add(hit.ImpactPoint);
		else
			OutH.Add(flat);

		DrawDebugSphere(world, OutH.Last(), 30.f, 8, FColor::Cyan, false, cachedRow.chargeDuration + 1.f);
	}

	// 세로 클론: 위쪽 바깥에 배치, 아래(-Y)를 바라봄, X축 슬라이스
	for (int32 i = 0; i < N; ++i)
	{
		const float x = Center.X - halfSpan + cachedRow.gridSpacing * i;
		const float y = Center.Y + halfSpan + cachedRow.cloneOffset;
		FVector flat(x, y, Center.Z);

		FHitResult hit;
		if (world->LineTraceSingleByChannel(hit, flat + FVector(0, 0, 800), flat - FVector(0, 0, 2000), ECC_WorldStatic))
			OutV.Add(hit.ImpactPoint);
		else
			OutV.Add(flat);

		DrawDebugSphere(world, OutV.Last(), 30.f, 8, FColor::Magenta, false, cachedRow.chargeDuration + 1.f);
	}
}

void UC_BossGridLaserPatternGA::SpawnClones(const TArray<FVector>& HPositions, const TArray<FVector>& VPositions)
{
	horizontalClones.Empty();
	verticalClones.Empty();

	const FVector hDir(1.f, 0.f, 0.f);   // +X (왼쪽→오른쪽)
	const FVector vDir(0.f, -1.f, 0.f);  // -Y (위→아래)

	UWorld* world = GetWorld();
	if (!world) return;

	for (const FVector& pos : HPositions)
	{
		AC_BossGhostClone* clone = world->SpawnActor<AC_BossGhostClone>(
			ghostCloneClass, pos, hDir.Rotation());
		if (clone)
		{
			clone->Initialize(this, hDir);
			if (chargeMontage) clone->PlayCharge(chargeMontage, cachedRow.chargeDuration);
			horizontalClones.Add(clone);
		}
	}

	for (const FVector& pos : VPositions)
	{
		AC_BossGhostClone* clone = world->SpawnActor<AC_BossGhostClone>(
			ghostCloneClass, pos, vDir.Rotation());
		if (clone)
		{
			clone->Initialize(this, vDir);
			if (chargeMontage) clone->PlayCharge(chargeMontage, cachedRow.chargeDuration);
			verticalClones.Add(clone);
		}
	}
}

void UC_BossGridLaserPatternGA::PickMarkedClones()
{
	if (horizontalClones.Num() > 0)
	{
		const int32 idx = FMath::RandRange(0, horizontalClones.Num() - 1);
		if (horizontalClones[idx].IsValid())
		{
			markedHorizontal = horizontalClones[idx];
			markedHorizontal->SetMarked(true);
		}
	}

	if (verticalClones.Num() > 0)
	{
		const int32 idx = FMath::RandRange(0, verticalClones.Num() - 1);
		if (verticalClones[idx].IsValid())
		{
			markedVertical = verticalClones[idx];
			markedVertical->SetMarked(true);
		}
	}
}

void UC_BossGridLaserPatternGA::OnChargeFinished()
{
	if (!cachedBossActor || !gridLaserClass) return;

	UWorld* world = GetWorld();
	if (!world) return;

	// 차지 완료 → 에너지 발사 모션 재생
	if (fireMontage)
	{
		for (TWeakObjectPtr<AC_BossGhostClone>& c : horizontalClones)
			if (c.IsValid()) c->PlayFire(fireMontage);
		for (TWeakObjectPtr<AC_BossGhostClone>& c : verticalClones)
			if (c.IsValid()) c->PlayFire(fireMontage);
	}

	spawnedLasers.Empty();
	int32 spawnCount = 0;

	auto SpawnLaserForClone = [&](TWeakObjectPtr<AC_BossGhostClone>& clonePtr)
	{
		if (!clonePtr.IsValid() || clonePtr->bCancelled) return;

		AC_GridLaser* laser = world->SpawnActor<AC_GridLaser>(
			gridLaserClass, clonePtr->GetActorLocation(), clonePtr->GetActorRotation());
		if (!laser) return;

		laser->Initialize(cachedMonsterASC, cachedBossActor, damageEffectClass,
			cachedRow.failureDamage, cachedRow.laserBeamLength, cachedRow.laserBeamWidth,
			cachedRow.laserPreviewTime, cachedRow.laserFireDuration);

		clonePtr->pairedLaser = laser;
		spawnedLasers.Add(laser);
		++spawnCount;
	};

	for (TWeakObjectPtr<AC_BossGhostClone>& c : horizontalClones) SpawnLaserForClone(c);
	for (TWeakObjectPtr<AC_BossGhostClone>& c : verticalClones)   SpawnLaserForClone(c);

	UE_LOG(LogTemp, Warning, TEXT("[GridLaserGA] OnChargeFinished. SpawnedLasers=%d"), spawnCount);

	const float totalDuration = cachedRow.laserPreviewTime + cachedRow.laserFireDuration + 0.5f;
	GetWorld()->GetTimerManager().SetTimer(
		patternEndTimerHandle, this, &UC_BossGridLaserPatternGA::OnPatternFinished,
		totalDuration, false);
}

void UC_BossGridLaserPatternGA::HandleMarkedGhostDestroyed(AC_BossGhostClone* DestroyedClone)
{
	if (!DestroyedClone) return;

	const bool bIsH = (markedHorizontal.Get() == DestroyedClone);
	const bool bIsV = (markedVertical.Get()   == DestroyedClone);

	UE_LOG(LogTemp, Warning, TEXT("[GridLaserGA] Marked clone destroyed (Line=%s)"),
		bIsH ? TEXT("H") : TEXT("V"));

	// 레이저가 이미 스폰됐으면 취소, 아직 차지 중이면 bCancelled=true로 스폰 스킵됨
	if (DestroyedClone->pairedLaser.IsValid())
		DestroyedClone->pairedLaser->CancelLaser();
}

void UC_BossGridLaserPatternGA::OnPatternFinished()
{
	UE_LOG(LogTemp, Warning, TEXT("[GridLaserGA] PatternFinished. Cleanup complete"));

	for (TWeakObjectPtr<AC_BossGhostClone>& c : horizontalClones)
		if (c.IsValid()) c->Destroy();
	for (TWeakObjectPtr<AC_BossGhostClone>& c : verticalClones)
		if (c.IsValid()) c->Destroy();

	horizontalClones.Empty();
	verticalClones.Empty();

	for (TWeakObjectPtr<AC_GridLaser>& l : spawnedLasers)
		if (l.IsValid()) l->Destroy();
	spawnedLasers.Empty();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UC_BossGridLaserPatternGA::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 타이머 정리 — 취소 시에도 타이머가 남지 않도록
	if (UWorld* world = GetWorld())
	{
		world->GetTimerManager().ClearTimer(chargeTimerHandle);
		world->GetTimerManager().ClearTimer(patternEndTimerHandle);
	}

	// 무적 태그 제거 — 정상 종료/취소 모두 대응
	if (cachedMonsterASC)
		cachedMonsterASC->RemoveLooseGameplayTag(
			FGameplayTag::RequestGameplayTag("State.Invincible"));

	// 카메라 복귀 — 정상 종료/취소/인터럽트 어떤 경우든 보장
	if (playerRef.IsValid())
		playerRef->SetTopDownCamera(false, cachedRow.cameraBlendTime,
			cachedRow.cameraTopDownArm, cachedRow.cameraTopDownPitch);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
