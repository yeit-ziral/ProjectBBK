// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BossStormPatternGA.h"

#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../../C_BossMonster.h"
#include "../../Object/C_BossStorm.h"

UC_BossStormPatternGA::UC_BossStormPatternGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer patternTags;
	patternTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Pattern.NonInterruptible"));
	SetAssetTags(patternTags);
}

void UC_BossStormPatternGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* boss = GetAvatarActorFromActorInfo();
	if (!boss)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ACharacter* bossChar = Cast<ACharacter>(boss);
	FVector bossLoc = boss->GetActorLocation();

	// 이동 차단 태그
	if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo())
		asc->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Boss.StormPattern")));

	// 소환 몽타주 재생 (stormDelay + stormLaunchDelay 전체에 맞춰 rate 조정)
	if (summoningMontage && bossChar)
	{
		float montageLen    = summoningMontage->GetPlayLength();
		float totalDuration = stormDelay + stormLaunchDelay;
		float rate = (montageLen > 0.f && totalDuration > 0.f) ? montageLen / totalDuration : 1.f;
		bossChar->PlayAnimMontage(summoningMontage, rate);
	}

	// MaxWalkSpeed = 0으로 이동 차단 (MovementMode는 Walking 유지)
	if (bossChar)
	{
		savedMaxWalkSpeed = bossChar->GetCharacterMovement()->MaxWalkSpeed;
		bossChar->GetCharacterMovement()->StopMovementImmediately();
		bossChar->GetCharacterMovement()->MaxWalkSpeed = 0.f;
	}

	// 스폰 위치 계산
	cachedBaseAngle   = FMath::RandRange(0.f, 360.f);
	cachedSpawnRadius = FMath::RandRange(patternRadius * 0.4f, patternRadius * 0.7f);

	FHitResult bossGroundHit;
	bool bBossHit = GetWorld()->LineTraceSingleByChannel(
		bossGroundHit,
		bossLoc + FVector(0, 0, 800.f),
		bossLoc - FVector(0, 0, 2000.f),
		ECC_WorldStatic);
	float bossGroundZ = bBossHit ? bossGroundHit.ImpactPoint.Z : bossLoc.Z;

	cachedSpawnPoints.Empty();
	cachedDecalActors.Empty();
	cachedStormActors.Empty();

	for (int32 i = 0; i < circleCount; ++i)
	{
		float angle = cachedBaseAngle + i * (360.f / circleCount);
		float rad   = FMath::DegreesToRadians(angle);

		FVector flatPoint(
			bossLoc.X + FMath::Cos(rad) * cachedSpawnRadius,
			bossLoc.Y + FMath::Sin(rad) * cachedSpawnRadius,
			bossLoc.Z);

		FHitResult pointHit;
		bool bPointHit = GetWorld()->LineTraceSingleByChannel(
			pointHit,
			flatPoint + FVector(0, 0, 800.f),
			flatPoint - FVector(0, 0, 2000.f),
			ECC_WorldStatic);

		FVector spawnPos = bPointHit ? pointHit.ImpactPoint : FVector(flatPoint.X, flatPoint.Y, bossGroundZ);
		cachedSpawnPoints.Add(spawnPos);

		if (magicCircleDecal)
		{
			if (UDecalComponent* decal = UGameplayStatics::SpawnDecalAtLocation(
				GetWorld(), magicCircleDecal, FVector(decalSize), spawnPos, FRotator(-90.f, 0, 0), stormDelay))
			{
				cachedDecalActors.Add(decal->GetOwner());
			}
		}
	}

	GetWorld()->GetTimerManager().SetTimer(spawnTimerHandle, this, &UC_BossStormPatternGA::SpawnStorms, stormDelay, false);
}

void UC_BossStormPatternGA::SpawnStorms()
{
	AC_BossMonster* boss = Cast<AC_BossMonster>(GetAvatarActorFromActorInfo());
	const float attackValue = boss ? static_cast<float>(boss->GetAttack()) * damageMultiplier : 0.f;
	UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo();

	for (const FVector& spawnPos : cachedSpawnPoints)
	{
		AActor* spawned = GetWorld()->SpawnActor<AActor>(stormActorClass, spawnPos, FRotator::ZeroRotator);
		if (AC_BossStorm* storm = Cast<AC_BossStorm>(spawned))
		{
			storm->InitProjectile(asc, damageEffectClass, attackValue, stormLaunchDelay, stormFlySpeed, stormMaxTravelDistance);
			cachedStormActors.Add(storm);
		}
	}

	// 폭풍이 실제로 날기 시작하는 시점(stormLaunchDelay 후)에 GA 종료
	// — 그 전까지는 태그가 살아있어 보스 일반공격이 차단됨
	GetWorld()->GetTimerManager().SetTimer(
		launchTimerHandle, this, &UC_BossStormPatternGA::OnStormsLaunched, stormLaunchDelay, false);
}

void UC_BossStormPatternGA::OnStormsLaunched()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UC_BossStormPatternGA::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AActor* boss = GetAvatarActorFromActorInfo())
	{
		if (ACharacter* bossChar = Cast<ACharacter>(boss))
		{
			bossChar->GetCharacterMovement()->MaxWalkSpeed = savedMaxWalkSpeed;
			if (summoningMontage) bossChar->StopAnimMontage(summoningMontage);
		}
	}

	if (UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo())
		asc->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Boss.StormPattern")));

	if (bWasCancelled)
	{
		for (TObjectPtr<AActor>& decalActor : cachedDecalActors)
			if (IsValid(decalActor)) decalActor->Destroy();

		for (TWeakObjectPtr<AActor>& stormActor : cachedStormActors)
			if (stormActor.IsValid()) stormActor->Destroy();
	}
	cachedDecalActors.Empty();
	cachedStormActors.Empty();

	if (UWorld* world = GetWorld())
	{
		world->GetTimerManager().ClearTimer(spawnTimerHandle);
		world->GetTimerManager().ClearTimer(launchTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
