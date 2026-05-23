// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

AC_BossProjectile::AC_BossProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(collision);
	collision->InitSphereRadius(15.f);
	// Pawn: 오버랩(플레이어 데미지), WorldStatic/Dynamic: 블록(지형 충돌)
	collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	collision->SetGenerateOverlapEvents(true);
	collision->SetNotifyRigidBodyCollision(true);
	collision->OnComponentBeginOverlap.AddDynamic(this, &AC_BossProjectile::OnSphereBeginOverlap);
	collision->OnComponentHit.AddDynamic(this, &AC_BossProjectile::OnSphereHit);

	mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	mesh->SetupAttachment(collision);
	mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	flightVFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FlightVFX"));
	flightVFX->SetupAttachment(collision);
	flightVFX->SetAutoActivate(true);

	projectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	projectileMovement->InitialSpeed = speed;
	projectileMovement->MaxSpeed = speed;
	projectileMovement->bRotationFollowsVelocity = true;
	projectileMovement->bShouldBounce = false;
	projectileMovement->ProjectileGravityScale = gravityScale;
}

void AC_BossProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(lifeSpan);
}

void AC_BossProjectile::InitProjectile(UAbilitySystemComponent* InInstigatorASC,
                                        TSubclassOf<UGameplayEffect> InDamageGEClass,
                                        float InDamage,
                                        const FVector& FireDirection)
{
	instigatorASC = InInstigatorASC;
	damageGEClass = InDamageGEClass;
	damageValue = InDamage;

	if (projectileMovement)
		projectileMovement->Velocity = FireDirection.GetSafeNormal() * speed;
}

void AC_BossProjectile::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                              bool bFromSweep, const FHitResult& SweepResult)
{
	if (bAlreadyHit || !OtherActor || OtherActor == GetOwner()) return;

	// 플레이어(IAbilitySystemInterface 구현체)에만 데미지 적용
	if (!Cast<IAbilitySystemInterface>(OtherActor)) return;

	bAlreadyHit = true;
	ApplyDamageToTarget(OtherActor);
	Destroy();
}

void AC_BossProjectile::OnSphereHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, FVector NormalImpulse,
                                     const FHitResult& Hit)
{
	// 지형·벽 충돌 시 즉시 소멸
	if (bAlreadyHit) return;
	bAlreadyHit = true;
	Destroy();
}

void AC_BossProjectile::ApplyDamageToTarget(AActor* Target)
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

	spec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag("Data.Damage"), damageValue);

	instigatorASC->ApplyGameplayEffectSpecToTarget(*spec.Data.Get(), targetASC);
}
