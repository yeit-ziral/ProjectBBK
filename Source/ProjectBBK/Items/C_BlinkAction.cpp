// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BlinkAction.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

void UC_BlinkAction::Execute_Implementation(UAbilitySystemComponent* ASC, AActor* AvatarActor)
{
	ACharacter* Character = Cast<ACharacter>(AvatarActor);
	if (!Character) return;

	UWorld* World = Character->GetWorld();
	if (!World) return;

	const FVector Start = Character->GetActorLocation();
	const FVector Direction = Character->GetActorForwardVector();
	const FVector End = Start + Direction * blinkDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	FHitResult Hit;
	const bool bBlockingHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);

	// 벽에 막히면 충돌 지점에서 캡슐 반경만큼 당겨 벽을 통과하지 않도록 함
	const float CapsuleRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector TargetLocation = bBlockingHit ? (Hit.ImpactPoint - Direction * CapsuleRadius) : End;

	Character->SetActorLocation(TargetLocation, false);
}
