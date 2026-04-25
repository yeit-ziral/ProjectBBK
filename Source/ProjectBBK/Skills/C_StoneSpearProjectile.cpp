// Fill out your copyright notice in the Description page of Project Settings.

#include "C_StoneSpearProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectTypes.h"
#include "../Monster/C_BaseMonster.h"

AC_StoneSpearProjectile::AC_StoneSpearProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 10.f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(30.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic,  ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn,         ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetNotifyRigidBodyCollision(true);

	ProjectileEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileEffect"));
	ProjectileEffect->SetupAttachment(Collision);
	ProjectileEffect->SetAutoActivate(true);

	ImpactEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ImpactEffect"));
	ImpactEffect->SetupAttachment(Collision);
	ImpactEffect->SetAutoActivate(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;
}

void AC_StoneSpearProjectile::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentBeginOverlap.AddDynamic(this, &AC_StoneSpearProjectile::OnSphereBeginOverlap);
	Collision->OnComponentHit.AddDynamic(this, &AC_StoneSpearProjectile::OnProjectileHit);
}

void AC_StoneSpearProjectile::InitProjectile(
	UAbilitySystemComponent* InInstigatorASC,
	AActor* InInstigatorActor,
	TSubclassOf<UGameplayEffect> InDamageGEClass,
	TSubclassOf<UGameplayEffect> InSlowedGEClass,
	float InDamage,
	const FVector& Direction)
{
	InstigatorASC   = InInstigatorASC;
	InstigatorActor = InInstigatorActor;
	DamageGEClass   = InDamageGEClass;
	SlowedGEClass   = InSlowedGEClass;
	DamageMagnitude = InDamage;

	ProjectileMovement->Velocity = Direction.GetSafeNormal() * ProjectileMovement->InitialSpeed;

	if (InstigatorActor.IsValid())
	{
		Collision->IgnoreActorWhenMoving(InstigatorActor.Get(), true);
	}
}

void AC_StoneSpearProjectile::OnSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bHasHit) return;
	if (!OtherActor || !OtherActor->IsA(AC_BaseMonster::StaticClass())) return;

	bHasHit = true;
	ApplyEffectsToTarget(OtherActor);
	HandleImpact();
}

void AC_StoneSpearProjectile::OnProjectileHit(
	UPrimitiveComponent* HitComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (bHasHit) return;

	bHasHit = true;
	HandleImpact();
}

void AC_StoneSpearProjectile::HandleImpact()
{
	ProjectileMovement->StopMovementImmediately();
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileEffect->Deactivate();
	ImpactEffect->Activate();
	SetLifeSpan(2.0f);
}

void AC_StoneSpearProjectile::ApplyEffectsToTarget(AActor* TargetActor)
{
	if (!InstigatorASC.IsValid() || !InstigatorActor.IsValid()) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC) return;

	FGameplayEffectContextHandle Context = InstigatorASC->MakeEffectContext();
	Context.AddInstigator(InstigatorActor.Get(), InstigatorActor.Get());

	if (DamageGEClass)
	{
		FGameplayEffectSpecHandle DamageSpec =
			InstigatorASC->MakeOutgoingSpec(DamageGEClass, 1.0f, Context);
		if (DamageSpec.IsValid())
		{
			FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
			DamageSpec.Data->SetSetByCallerMagnitude(DamageTag, DamageMagnitude);
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data, TargetASC);
		}
	}

	if (SlowedGEClass)
	{
		FGameplayEffectSpecHandle SlowedSpec =
			InstigatorASC->MakeOutgoingSpec(SlowedGEClass, 1.0f, Context);
		if (SlowedSpec.IsValid())
		{
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*SlowedSpec.Data, TargetASC);
		}
	}
}
