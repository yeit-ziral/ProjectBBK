// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectBBK/PlayerCharacter/C_RangeCharacter.h"

AC_RangeCharacter::AC_RangeCharacter(const class FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AC_RangeCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AC_RangeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateAimOffset();
}

void AC_RangeCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AC_RangeCharacter::UpdateAimOffset()
{
	if (AController *ctrl = GetController())
	{
		FRotator aimRot;
		FVector _;
		ctrl->GetPlayerViewPoint(_, aimRot);

		// Pitch
		aimPitch = FMath::ClampAngle(FRotator::NormalizeAxis(aimRot.Pitch), -90.0f, 90.0f);

		// Yaw
		const FRotator delta = (aimRot - GetActorRotation()).GetNormalized();
		aimYaw = FMath::ClampAngle(delta.Yaw, -90.0f, 90.0f);
	}
}