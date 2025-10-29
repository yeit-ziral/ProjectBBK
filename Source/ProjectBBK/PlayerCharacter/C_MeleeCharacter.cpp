// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectBBK/PlayerCharacter/C_MeleeCharacter.h"

AC_MeleeCharacter::AC_MeleeCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FObjectFinder<USkeletalMesh> mesh(L"/Script/Engine.SkeletalMesh'/Game/ParagonGreystone/Characters/Heroes/Greystone/Skins/Tough/Meshes/Greystone_Tough.Greystone_Tough'");

	GetMesh()->SetSkeletalMesh(mesh.Object);

	GetMesh()->SetRelativeLocation(FVector(0, 0, -90.0f));
	GetMesh()->SetRelativeRotation(FRotator(0, -90.0f, 0));
	GetMesh()->SetRelativeScale3D(FVector(0.9f, 0.9f, 0.9f));
}

void AC_MeleeCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AC_MeleeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AC_MeleeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}