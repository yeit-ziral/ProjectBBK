// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectBBK/PlayerCharacter/C_RangeCharacter.h"

AC_RangeCharacter::AC_RangeCharacter(const class FObjectInitializer& ObjectInitializer)
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
}

void AC_RangeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
