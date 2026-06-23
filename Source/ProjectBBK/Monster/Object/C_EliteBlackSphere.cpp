// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EliteBlackSphere.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

AC_EliteBlackSphere::AC_EliteBlackSphere()
{
	PrimaryActorTick.bCanEverTick = false;

	collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(collision);
	collision->InitSphereRadius(40.f);
	// 플레이어(Pawn)와 오버랩만 — 월드/타 몬스터는 통과
	collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	collision->SetGenerateOverlapEvents(true);

	sphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereMesh"));
	sphereMesh->SetupAttachment(collision);
	sphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	trailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
	trailEffect->SetupAttachment(collision);
	trailEffect->bAutoActivate = true;

	projectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	projectileMovement->InitialSpeed = 1200.f;
	projectileMovement->MaxSpeed = 1200.f;
	projectileMovement->bRotationFollowsVelocity = true;
	projectileMovement->bShouldBounce = false;
	projectileMovement->ProjectileGravityScale = 0.f;

	collision->OnComponentBeginOverlap.AddDynamic(this, &AC_EliteBlackSphere::OnSphereOverlap);
}

void AC_EliteBlackSphere::BeginPlay()
{
	Super::BeginPlay();
}

void AC_EliteBlackSphere::InitSphere(UAbilitySystemComponent* InInstigatorASC, AActor* InInstigatorActor,
	TSubclassOf<UGameplayEffect> InDamageGE, float InDamage,
	const FVector& Direction, FGameplayTag InHitEventTag)
{
	instigatorASC   = InInstigatorASC;
	instigatorActor = InInstigatorActor;
	damageGEClass   = InDamageGE;
	damageValue     = InDamage;
	hitEventTag     = InHitEventTag;

	if (InInstigatorActor)
		collision->IgnoreActorWhenMoving(InInstigatorActor, true);

	if (projectileMovement)
		projectileMovement->Velocity = Direction.GetSafeNormal() * projectileMovement->InitialSpeed;

	SetLifeSpan(lifeSpanSeconds);
}

void AC_EliteBlackSphere::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bAlreadyHit || !OtherActor || OtherActor == instigatorActor.Get()) return;

	// 플레이어만 대상 (타 몬스터 Pawn 제외)
	if (OtherActor != UGameplayStatics::GetPlayerPawn(this, 0)) return;

	bAlreadyHit = true;

	ApplyDamageToTarget(OtherActor);

	// 시전 몬스터에게 명중 이벤트 전달 → GA가 끌어오기 처리 (Target = 플레이어)
	if (hitEventTag.IsValid() && instigatorActor.IsValid())
	{
		FGameplayEventData payload;
		payload.EventTag   = hitEventTag;
		payload.Instigator = this;
		payload.Target     = OtherActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			instigatorActor.Get(), hitEventTag, payload);
	}

	Destroy();
}

void AC_EliteBlackSphere::ApplyDamageToTarget(AActor* Target)
{
	if (!instigatorASC.IsValid() || !damageGEClass || !Target) return;

	UAbilitySystemComponent* targetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!targetASC) return;

	FGameplayEffectContextHandle context = instigatorASC->MakeEffectContext();
	context.AddInstigator(instigatorActor.Get(), instigatorActor.Get());

	FGameplayEffectSpecHandle spec = instigatorASC->MakeOutgoingSpec(damageGEClass, 1.f, context);
	if (!spec.IsValid()) return;

	spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), damageValue);
	instigatorASC->ApplyGameplayEffectSpecToTarget(*spec.Data.Get(), targetASC);
}
