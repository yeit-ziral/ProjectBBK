// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BossStorm.h"

#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

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

	// 발사 전까지 Tick 비활성
	SetActorTickEnabled(false);
}

void AC_BossStorm::InitProjectile(UAbilitySystemComponent* InInstigatorASC,
                                   TSubclassOf<UGameplayEffect> InDamageGEClass,
                                   float InDamageValue,
                                   float inLaunchDelay, float inFlySpeed, float inMaxTravelDistance)
{
	instigatorASC     = InInstigatorASC;
	damageGEClass     = InDamageGEClass;
	damageValue       = InDamageValue;
	flySpeed          = inFlySpeed;
	maxTravelDistance = inMaxTravelDistance;

	GetWorld()->GetTimerManager().SetTimer(launchTimerHandle, this, &AC_BossStorm::LaunchTowardPlayer, inLaunchDelay, false);
}

void AC_BossStorm::LaunchTowardPlayer()
{
	APawn* player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!player)
	{
		Destroy();
		return;
	}

	FVector toPlayer = player->GetActorLocation() - GetActorLocation();
	toPlayer.Z = 0.f;
	flyDirection = toPlayer.GetSafeNormal();

	SetActorRotation(flyDirection.Rotation());

	bFlying = true;
	SetActorTickEnabled(true);
}

void AC_BossStorm::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bFlying) return;

	float step = flySpeed * DeltaTime;
	distanceTraveled += step;

	if (distanceTraveled >= maxTravelDistance)
	{
		Destroy();
		return;
	}

	FVector newLoc = GetActorLocation() + flyDirection * step;

	// 지면 추적 — 경사면에서도 지면에 붙어서 이동
	FHitResult hit;
	if (GetWorld()->LineTraceSingleByChannel(
		hit,
		newLoc + FVector(0, 0, 500.f),
		newLoc - FVector(0, 0, 1000.f),
		ECC_WorldStatic))
	{
		newLoc.Z = hit.ImpactPoint.Z;
	}

	SetActorLocation(newLoc);
}

void AC_BossStorm::OnDamageSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bHit || !OtherActor) return;

	// 플레이어 캐릭터에만 반응
	APawn* pawn = Cast<APawn>(OtherActor);
	if (!pawn || !pawn->IsPlayerControlled()) return;

	IAbilitySystemInterface* ascInterface = Cast<IAbilitySystemInterface>(OtherActor);
	if (!ascInterface) return;

	UAbilitySystemComponent* targetASC = ascInterface->GetAbilitySystemComponent();
	if (!targetASC || !instigatorASC.IsValid() || !damageGEClass) return;

	bHit = true;

	FGameplayEffectContextHandle context = instigatorASC->MakeEffectContext();
	context.AddInstigator(GetOwner(), GetOwner());
	FGameplayEffectSpecHandle spec = instigatorASC->MakeOutgoingSpec(damageGEClass, 1.f, context);
	if (spec.IsValid())
	{
		spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), damageValue);
		instigatorASC->ApplyGameplayEffectSpecToTarget(*spec.Data.Get(), targetASC);
	}

	Destroy();
}
