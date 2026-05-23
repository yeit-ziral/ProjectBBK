// Fill out your copyright notice in the Description page of Project Settings.


#include "C_RangedProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

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
	projectileMovement->ProjectileGravityScale = 0.f; // ���� ź

	collision->SetGenerateOverlapEvents(true);
	collision->OnComponentBeginOverlap.AddDynamic(this, &AC_RangedProjectile::OnSphereBeginOverlap);
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

void AC_RangedProjectile::InitProjectile(UAbilitySystemComponent* InInstigatorASC,
                                          TSubclassOf<UGameplayEffect> InDamageGEClass,
                                          float InDamage,
                                          const FVector& Direction)
{
	instigatorASC = InInstigatorASC;
	damageGEClass = InDamageGEClass;
	damageValue   = InDamage;

	if (projectileMovement)
		projectileMovement->Velocity = Direction.GetSafeNormal() * projectileMovement->InitialSpeed;
}

void AC_RangedProjectile::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                                bool bFromSweep, const FHitResult& SweepResult)
{
	if (bAlreadyHit || !OtherActor || OtherActor == GetOwner()) return;
	if (!Cast<IAbilitySystemInterface>(OtherActor)) return;

	bAlreadyHit = true;
	ApplyDamageToTarget(OtherActor);
	Destroy();
}

void AC_RangedProjectile::ApplyDamageToTarget(AActor* Target)
{
	if (!instigatorASC.IsValid()) return;
	if (!damageGEClass) return;

	IAbilitySystemInterface* ascInterface = Cast<IAbilitySystemInterface>(Target);
	if (!ascInterface) return;

	UAbilitySystemComponent* targetASC = ascInterface->GetAbilitySystemComponent();
	if (!targetASC) return;

	FGameplayEffectContextHandle context = instigatorASC->MakeEffectContext();
	context.AddInstigator(GetOwner(), GetOwner());

	FGameplayEffectSpecHandle spec = instigatorASC->MakeOutgoingSpec(damageGEClass, 1.f, context);
	if (!spec.IsValid()) return;

	spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), damageValue);
	instigatorASC->ApplyGameplayEffectSpecToTarget(*spec.Data.Get(), targetASC);
}

// Called every frame
void AC_RangedProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

