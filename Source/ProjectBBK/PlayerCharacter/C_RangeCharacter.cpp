// Fill out your copyright notice in the Description page of Project Settings.

#include "C_RangeCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Monster/C_BaseMonster.h"

AC_RangeCharacter::AC_RangeCharacter(const class FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

void AC_RangeCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AC_RangeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AC_RangeCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

AC_BaseMonster *AC_RangeCharacter::GetHighestPriorityTarget() const
{
	TArray<AActor *> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor *> ActorsToIgnore;
	ActorsToIgnore.Add(const_cast<AC_RangeCharacter *>(this));

	UKismetSystemLibrary::SphereOverlapActors(
		this,
		GetActorLocation(),
		detectionRange,
		ObjectTypes,
		AC_BaseMonster::StaticClass(),
		ActorsToIgnore,
		OverlappedActors);

	AC_BaseMonster *BestTarget = nullptr;
	float BestDistSq = FLT_MAX;

	for (AActor *Actor : OverlappedActors)
	{
		AC_BaseMonster *Monster = Cast<AC_BaseMonster>(Actor);
		if (!Monster || Monster->GetcurHP() <= 0)
			continue;

		float DistSq = FVector::DistSquared(GetActorLocation(), Monster->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Monster;
		}
	}

	return BestTarget;
}

void AC_RangeCharacter::FaceTarget(AActor *Target)
{
	if (!Target)
		return;

	FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
		SetActorRotation(Direction.Rotation());
}
