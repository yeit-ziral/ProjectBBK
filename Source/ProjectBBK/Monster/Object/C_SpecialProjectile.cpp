// Fill out your copyright notice in the Description page of Project Settings.

#include "C_SpecialProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

AC_SpecialProjectile::AC_SpecialProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(collision);
	collision->InitSphereRadius(30.f);
	collision->SetCollisionProfileName(TEXT("Projectile"));

	// 미사일 메쉬
	missileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MissileMesh"));
	missileMesh->SetupAttachment(collision);
	missileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 트레일 (Cascade)
	trailEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailEffect"));
	trailEffect->SetupAttachment(collision);
	trailEffect->SetAutoActivate(true);
	trailEffect->SetBoundsScale(10.f);
	trailSystem = nullptr;
}

void AC_SpecialProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void AC_SpecialProjectile::InitSpecialProjectile(FVector InTargetLocation, float InArcHeight, float InMoveSpeed)
{
	targetLocation = InTargetLocation;
	moveSpeed = InMoveSpeed;

	// 정점: 스폰 위치와 타겟의 중점 상공
	const FVector midpoint = (GetActorLocation() + targetLocation) * 0.5f;
	apexLocation = midpoint + FVector(0.f, 0.f, InArcHeight);

	currentPhase = ESpecialProjectilePhase::Rising;
	bInitialized = true;

	// 트레일 시작
	if (trailEffect)
	{
		if (trailSystem)
			trailEffect->SetTemplate(trailSystem);
		trailEffect->Activate(true);
	}
}

void AC_SpecialProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bInitialized)
		return;

	switch (currentPhase)
	{
	case ESpecialProjectilePhase::Rising:
		MoveToward(apexLocation, DeltaTime);
		// 메쉬 +Z(탄두)가 하늘을 향하도록 — 기본 자세 그대로
		SetActorRotation(FRotator::ZeroRotator);
		if (FVector::Dist(GetActorLocation(), apexLocation) < ArrivalThreshold)
		{
			currentPhase = ESpecialProjectilePhase::Falling;
		}
		break;

	case ESpecialProjectilePhase::Falling:
	{
		MoveToward(targetLocation, DeltaTime);
		// 메쉬 +Z(탄두)가 타겟을 향하도록
		const FVector dirToTarget = (targetLocation - GetActorLocation()).GetSafeNormal();
		if (!dirToTarget.IsNearlyZero())
		{
			SetActorRotation(FRotationMatrix::MakeFromZ(dirToTarget).Rotator());
		}
		if (FVector::Dist(GetActorLocation(), targetLocation) < ArrivalThreshold)
		{
			bInitialized = false;
			if (trailEffect)
				trailEffect->Deactivate();
			if (explosionEffect)
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), explosionEffect, targetLocation, FRotator::ZeroRotator, FVector(0.5f));
			if (explosionSound)
				UGameplayStatics::PlaySoundAtLocation(this, explosionSound, targetLocation);
			ApplyAoEDamage();
			OnReachedTarget();
			Destroy();
		}
		break;
	}
	}
}

void AC_SpecialProjectile::InitGASDamage(UAbilitySystemComponent* InInstigatorASC,
                                          TSubclassOf<UGameplayEffect> InDamageGEClass,
                                          float InDamage,
                                          float InAoERadius)
{
	instigatorASC = InInstigatorASC;
	damageGEClass = InDamageGEClass;
	damageValue   = InDamage;
	aoERadius     = InAoERadius;
}

void AC_SpecialProjectile::ApplyAoEDamage()
{
	if (!instigatorASC.IsValid() || !damageGEClass) return;

	TArray<TEnumAsByte<EObjectTypeQuery>> objectTypes;
	objectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ignoredActors;
	if (GetOwner()) ignoredActors.Add(GetOwner());

	TArray<AActor*> hitActors;
	UKismetSystemLibrary::SphereOverlapActors(
		this, targetLocation, aoERadius, objectTypes, nullptr, ignoredActors, hitActors);

	for (AActor* hitActor : hitActors)
	{
		IAbilitySystemInterface* ascInterface = Cast<IAbilitySystemInterface>(hitActor);
		if (!ascInterface) continue;

		UAbilitySystemComponent* targetASC = ascInterface->GetAbilitySystemComponent();
		if (!targetASC) continue;

		FGameplayEffectContextHandle context = instigatorASC->MakeEffectContext();
		context.AddInstigator(GetOwner(), GetOwner());

		FGameplayEffectSpecHandle spec = instigatorASC->MakeOutgoingSpec(damageGEClass, 1.f, context);
		if (!spec.IsValid()) continue;

		spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), damageValue);
		instigatorASC->ApplyGameplayEffectSpecToTarget(*spec.Data.Get(), targetASC);
	}
}

void AC_SpecialProjectile::MoveToward(FVector Destination, float DeltaTime)
{
	const FVector direction = (Destination - GetActorLocation()).GetSafeNormal();
	// bSweep=false: 비행 중 지형/콜리전에 막히지 않도록
	SetActorLocation(GetActorLocation() + direction * moveSpeed * DeltaTime, false);
	// 회전은 Tick에서 페이즈별로 별도 처리
}
