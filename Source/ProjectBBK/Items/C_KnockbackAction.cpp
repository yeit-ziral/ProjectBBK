// Fill out your copyright notice in the Description page of Project Settings.

#include "C_KnockbackAction.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "../Monster/C_BaseMonster.h"

void UC_KnockbackAction::Execute_Implementation(UAbilitySystemComponent* ASC, AActor* AvatarActor)
{
	if (!AvatarActor) return;

	UWorld* World = AvatarActor->GetWorld();
	if (!World) return;

	// Sphere Overlap — Pawn 채널, AC_BaseMonster 필터 (C_RangedUltimate::HandleNotifyEvent와 동일 스타일, Box 대신 Sphere)
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		AvatarActor->GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(radius),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Cast<AC_BaseMonster>(Target))
		{
			continue;
		}

		ACharacter* TargetCharacter = Cast<ACharacter>(Target);
		if (!TargetCharacter)
		{
			continue;
		}

		// 넉백 면역 태그 체크
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (TargetASC && TargetASC->HasMatchingGameplayTag(
			FGameplayTag::RequestGameplayTag(FName("State.KnockbackImmune"))))
		{
			continue;
		}

		// AI 이동 중단 — BT가 즉시 이동 명령을 재발행해 넉백을 상쇄하지 않도록
		if (AAIController* AIC = Cast<AAIController>(TargetCharacter->GetController()))
		{
			AIC->StopMovement();
		}

		FVector Direction = Target->GetActorLocation() - AvatarActor->GetActorLocation();
		Direction.Z = 0.f;
		Direction.Normalize();

		TargetCharacter->LaunchCharacter(Direction * knockbackForce, true, false);
	}
}
