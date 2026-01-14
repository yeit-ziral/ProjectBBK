// Fill out your copyright notice in the Description page of Project Settings.


#include "C_RangedProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AC_RangedProjectile::AC_RangedProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;


	// collision
	collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(collision);
	collision->InitSphereRadius(10.f);
	collision->SetCollisionProfileName(TEXT("Projectile"));

	// Particle
	particle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Particle"));
	particle->SetupAttachment(collision);
	particle->SetAutoActivate(true);

	// Projectile Movement
	projectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	projectileMovement->InitialSpeed = 2000.f;
	projectileMovement->MaxSpeed = 2000.f;
	projectileMovement->bRotationFollowsVelocity = true;
	projectileMovement->bShouldBounce = false;
	projectileMovement->ProjectileGravityScale = 0.f; // Á÷¼± Åº

}

// Called when the game starts or when spawned
void AC_RangedProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

void AC_RangedProjectile::InitVelocity(const FVector& Direction)
{
	if (!projectileMovement) return;

	const FVector dir = Direction.GetSafeNormal();
	projectileMovement->Velocity = dir * projectileMovement->InitialSpeed;
}

// Called every frame
void AC_RangedProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

