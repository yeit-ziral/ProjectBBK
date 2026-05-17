// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossStorm.h"

#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

AC_BossStorm::AC_BossStorm()
{
	PrimaryActorTick.bCanEverTick = true;

	root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(root);

	damageSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DamageSphere"));
	damageSphere->SetupAttachment(root);
	damageSphere->SetSphereRadius(200.f);
	damageSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	stormParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("StormEffect"));
	stormParticle->SetupAttachment(root);

	damageSphere->OnComponentBeginOverlap.AddDynamic(this, &AC_BossStorm::OnDamageSphereBeginOverlap);
}

void AC_BossStorm::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(stormLifeTime);
}

void AC_BossStorm::InitOrbit(const FVector& InCenter, float InRadius, float InStartAngleDeg, float InSpeed)
{
	bOrbit = true;
	orbitCenter = InCenter;
	orbitRadius = InRadius;
	orbitAngle = InStartAngleDeg;
	orbitSpeed = InSpeed;
}

void AC_BossStorm::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bOrbit) return;

	orbitAngle += orbitSpeed * DeltaTime;

	float Rad = FMath::DegreesToRadians(orbitAngle);
	float newX = orbitCenter.X + FMath::Cos(Rad) * orbitRadius;
	float newY = orbitCenter.Y + FMath::Sin(Rad) * orbitRadius;

	// 현재 위치 아래 지면 탐색 → 지형이 고르지 않아도 항상 지면에 붙음
	FHitResult Hit;
	float groundZ = orbitCenter.Z;
	if (GetWorld()->LineTraceSingleByChannel(
		Hit,
		FVector(newX, newY, orbitCenter.Z + 500.f),
		FVector(newX, newY, orbitCenter.Z - 1000.f),
		ECC_WorldStatic))
	{
		groundZ = Hit.ImpactPoint.Z;
	}

	SetActorLocation(FVector(newX, newY, groundZ));
}

void AC_BossStorm::OnDamageSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	IAbilitySystemInterface* ascInterface = Cast<IAbilitySystemInterface>(OtherActor);
	if (!ascInterface) return;

	UAbilitySystemComponent* abilitySystemComponent = ascInterface->GetAbilitySystemComponent();
	if (!abilitySystemComponent || !damageEffect) return;

	FGameplayEffectContextHandle context = abilitySystemComponent->MakeEffectContext();
	context.AddSourceObject(this);

	FGameplayEffectSpecHandle spec = abilitySystemComponent->MakeOutgoingSpec(damageEffect, 1.f, context);
	if (spec.IsValid())
	{
		abilitySystemComponent->ApplyGameplayEffectSpecToSelf(*spec.Data.Get());
	}
}
