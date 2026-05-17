// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossStormPatternGA.h"

#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"
#include "../../C_BossMonster.h"
#include "../../Object/C_BossStorm.h"

UC_BossStormPatternGA::UC_BossStormPatternGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 발동 중 그로기 인터럽트 면제 — 스톰 액터는 GA 종료 후에도 독립 동작하므로 반드시 완주해야 함
	FGameplayTagContainer patternTags;
	patternTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Pattern.NonInterruptible"));
	SetAssetTags(patternTags);
}

void UC_BossStormPatternGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* BossActor = GetAvatarActorFromActorInfo();
	if (!BossActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FVector BossLoc = BossActor->GetActorLocation();

	// 공전 반지름 랜덤 결정
	cachedOrbitRadius = FMath::RandRange(patternRadius * 0.4f, patternRadius * 0.7f);
	cachedBaseAngle = FMath::RandRange(0.f, 360.f);

	// 오빗 중심: 보스 XY + 보스 위치 기준 지면 Z
	FHitResult BossGroundHit;
	bool bBossHit = GetWorld()->LineTraceSingleByChannel(
		BossGroundHit,
		BossLoc + FVector(0, 0, 800.f),
		BossLoc - FVector(0, 0, 2000.f),
		ECC_WorldStatic
	);
	float bossGroundZ = bBossHit ? BossGroundHit.ImpactPoint.Z : BossLoc.Z;
	cachedOrbitCenter = FVector(BossLoc.X, BossLoc.Y, bossGroundZ);

	// 각 오빗 포인트마다 개별 지면 탐색 → 데칼/스폰 위치 정확한 지면에 고정
	cachedSpawnPoints.Empty();
	for (int32 i = 0; i < circleCount; ++i)
	{
		float Angle = cachedBaseAngle + i * (360.f / circleCount);
		float Rad = FMath::DegreesToRadians(Angle);

		FVector FlatPoint(
			BossLoc.X + FMath::Cos(Rad) * cachedOrbitRadius,
			BossLoc.Y + FMath::Sin(Rad) * cachedOrbitRadius,
			BossLoc.Z
		);

		FHitResult PointHit;
		bool bPointHit = GetWorld()->LineTraceSingleByChannel(
			PointHit,
			FlatPoint + FVector(0, 0, 800.f),
			FlatPoint - FVector(0, 0, 2000.f),
			ECC_WorldStatic
		);

		FVector SpawnPos = bPointHit ? PointHit.ImpactPoint : FVector(FlatPoint.X, FlatPoint.Y, bossGroundZ);
		cachedSpawnPoints.Add(SpawnPos);

		if (magicCircleDecal)
		{
			if (UDecalComponent* decal = UGameplayStatics::SpawnDecalAtLocation(
				GetWorld(), magicCircleDecal, FVector(decalSize), SpawnPos, FRotator(-90.f, 0, 0), stormDelay))
			{
				cachedDecalActors.Add(decal->GetOwner());
			}
		}
	}

	FTimerHandle StormTimer;
	GetWorld()->GetTimerManager().SetTimer(StormTimer, this, &UC_BossStormPatternGA::SpawnStorms, stormDelay, false);

	UE_LOG(LogTemp, Warning, TEXT("StormPattern: Orbit pattern armed, delay=%.1f, count=%d, radius=%.0f, speed=%.0f deg/s"),
		stormDelay, circleCount, cachedOrbitRadius, orbitSpeed);
}

void UC_BossStormPatternGA::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 취소(보스 사망 등)로 데칼이 남아있을 경우 즉시 제거
	if (bWasCancelled)
	{
		for (TObjectPtr<AActor>& decalActor : cachedDecalActors)
		{
			if (IsValid(decalActor))
				decalActor->Destroy();
		}
	}
	cachedDecalActors.Empty();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UC_BossStormPatternGA::SpawnStorms()
{
	for (int32 i = 0; i < cachedSpawnPoints.Num(); ++i)
	{
		AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(stormActorClass, cachedSpawnPoints[i], FRotator::ZeroRotator);
		if (AC_BossStorm* Storm = Cast<AC_BossStorm>(SpawnedActor))
		{
			float StartAngle = cachedBaseAngle + i * (360.f / circleCount);
			Storm->InitOrbit(cachedOrbitCenter, cachedOrbitRadius, StartAngle, orbitSpeed);
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
