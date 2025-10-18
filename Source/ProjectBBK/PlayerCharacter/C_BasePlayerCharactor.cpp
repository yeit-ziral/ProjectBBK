// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BasePlayerCharactor.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AC_BasePlayerCharactor::AC_BasePlayerCharactor()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	springArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	springArm->SetupAttachment(RootComponent); 

	camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	camera->SetupAttachment(springArm);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
}

// Called when the game starts or when spawned
void AC_BasePlayerCharactor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AC_BasePlayerCharactor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;

	if (!Velocity.IsNearlyZero())
	{
		FRotator TargetRotation = Velocity.Rotation();
		FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 10.0f);
		SetActorRotation(NewRotation);
	}
}

// Called to bind functionality to input
void AC_BasePlayerCharactor::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 1. Moving Bind (W/S & A/D)
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AC_BasePlayerCharactor::MyMoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AC_BasePlayerCharactor::MyMoveRight);

	// 2. Camera Rottation Bind (Mouse X/Y)
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput); // APawn의 기본 함수 사용
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput); // APawn의 기본 함수 사용
}

void AC_BasePlayerCharactor::MyMoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		FRotator Rotation = Controller->GetControlRotation();
		FRotator YawRotation(0, Rotation.Yaw, 0);

		FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AC_BasePlayerCharactor::MyMoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}