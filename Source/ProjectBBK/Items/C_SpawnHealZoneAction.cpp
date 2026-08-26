// Fill out your copyright notice in the Description page of Project Settings.

#include "C_SpawnHealZoneAction.h"
#include "../Skills/C_HealZone.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

void UC_SpawnHealZoneAction::Execute_Implementation(UAbilitySystemComponent* ASC, AActor* AvatarActor)
{
	if (!ASC || !AvatarActor || !zoneClass) return;

	UWorld* World = AvatarActor->GetWorld();
	if (!World) return;

	// 3인칭 카메라 지면 위치 탐색 패턴 (docs/patterns.md) — AvatarActor 발밑 지면을 LineTrace로 탐색
	const FVector ActorLoc = AvatarActor->GetActorLocation();
	const FVector Start = ActorLoc + FVector(0.f, 0.f, 500.f);
	const FVector End = ActorLoc - FVector(0.f, 0.f, 500.f);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(AvatarActor);

	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return;   // 지면을 못 찾으면 스폰하지 않음 (Debugging Checklist #22)
	}

	AC_HealZone* Zone = World->SpawnActor<AC_HealZone>(zoneClass, Hit.ImpactPoint, FRotator::ZeroRotator);
	if (!Zone) return;

	Zone->Initialize(ASC, AvatarActor, tickEffectClass, radius, zoneLifetime, tickInterval, healPerTick);
}
