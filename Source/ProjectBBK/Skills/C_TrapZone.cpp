// Fill out your copyright notice in the Description page of Project Settings.

#include "C_TrapZone.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "../Monster/C_BaseMonster.h"

AC_TrapZone::AC_TrapZone()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TriggerCapsule"));
	TriggerCapsule->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	RootComponent = TriggerCapsule;

	DecalComp = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComp"));
	DecalComp->SetupAttachment(RootComponent);

	PointLightComp = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLightComp"));
	PointLightComp->SetupAttachment(RootComponent);
}

void AC_TrapZone::BeginPlay()
{
	Super::BeginPlay();

	TriggerCapsule->OnComponentBeginOverlap.AddDynamic(this, &AC_TrapZone::OnTriggerOverlap);
}

void AC_TrapZone::InitTrap(
	UAbilitySystemComponent* InASC,
	float InDamageMagnitude,
	float InGroggyMagnitude,
	float InLifeTime,
	float InTriggerRadius,
	float InTriggerHalfHeight,
	float InDamageRadius,
	float InDamageHalfHeight)
{
	OwnerASC               = InASC;
	DamageMagnitude        = InDamageMagnitude;
	GroggyMagnitude        = InGroggyMagnitude;
	LifeTime               = InLifeTime;
	TriggerCapsuleRadius      = InTriggerRadius;
	TriggerCapsuleHalfHeight  = InTriggerHalfHeight;
	DamageCapsuleRadius       = InDamageRadius;
	DamageCapsuleHalfHeight   = InDamageHalfHeight;

	TriggerCapsule->SetCapsuleSize(TriggerCapsuleRadius, TriggerCapsuleHalfHeight);

	if (IsValid(DecalMaterial))
	{
		DecalComp->SetDecalMaterial(DecalMaterial);
	}

	GetWorldTimerManager().SetTimer(
		LifeTimerHandle,
		this, &AC_TrapZone::OnLifeTimeExpired,
		LifeTime, false);

#if !UE_BUILD_SHIPPING
	DrawDebugCapsule(
		GetWorld(),
		GetActorLocation(),
		TriggerCapsuleHalfHeight,
		TriggerCapsuleRadius,
		FQuat::Identity,
		FColor::Green,
		true,
		-1.f,
		0,
		2.0f);
#endif
}

void AC_TrapZone::OnTriggerOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bTriggered)
	{
		return;
	}

	if (!Cast<AC_BaseMonster>(OtherActor))
	{
		return;
	}

	bTriggered = true;

	GetWorldTimerManager().ClearTimer(LifeTimerHandle);

#if !UE_BUILD_SHIPPING
	FlushPersistentDebugLines(GetWorld());
#endif

	DecalComp->SetVisibility(false);
	PointLightComp->SetVisibility(false);

	if (IsValid(ImpactSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	if (IsValid(ImpactNiagaraSystem))
	{
		UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactNiagaraSystem,
			GetActorLocation(),
			GetActorRotation());

		if (IsValid(NiagaraComp))
		{
			NiagaraComp->SetWorldScale3D(NiagaraScale);
		}
	}

	ApplyDamageToOverlappingMonsters();

	GetWorldTimerManager().SetTimer(
		LifeTimerHandle,
		this, &AC_TrapZone::DestroyTrap,
		4.5f, false);
}

void AC_TrapZone::ApplyDamageToOverlappingMonsters()
{
	if (!OwnerASC.IsValid())
	{
		return;
	}

	if (!IsValid(DamageEffectClass))
	{
		return;
	}

	TArray<AActor*> OverlappedActors;
	UKismetSystemLibrary::CapsuleOverlapActors(
		GetWorld(),
		GetActorLocation(),
		DamageCapsuleRadius,
		DamageCapsuleHalfHeight,
		TArray<TEnumAsByte<EObjectTypeQuery>>{ UEngineTypes::ConvertToObjectType(ECC_Pawn) },
		AC_BaseMonster::StaticClass(),
		TArray<AActor*>{},
		OverlappedActors);

#if !UE_BUILD_SHIPPING
	DrawDebugCapsule(
		GetWorld(),
		GetActorLocation(),
		DamageCapsuleHalfHeight,
		DamageCapsuleRadius,
		FQuat::Identity,
		FColor::Red,
		false,
		3.0f,
		0,
		2.0f);
#endif

	FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(
		DamageEffectClass, 1.0f,
		OwnerASC->MakeEffectContext());

	if (!SpecHandle.IsValid())
	{
		return;
	}

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		FGameplayTag::RequestGameplayTag(TEXT("Data.Damage")),
		DamageMagnitude);

	FGameplayEffectSpecHandle GroggySpecHandle;
	if (IsValid(GroggyEffectClass))
	{
		GroggySpecHandle = OwnerASC->MakeOutgoingSpec(
			GroggyEffectClass, 1.0f,
			OwnerASC->MakeEffectContext());

		if (GroggySpecHandle.IsValid())
		{
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
				GroggySpecHandle,
				FGameplayTag::RequestGameplayTag(TEXT("Data.GroggyDamage")),
				GroggyMagnitude);
		}
	}

	for (AActor* Actor : OverlappedActors)
	{
		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);

		if (IsValid(TargetASC))
		{
			OwnerASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

			if (GroggySpecHandle.IsValid())
			{
				OwnerASC->ApplyGameplayEffectSpecToTarget(*GroggySpecHandle.Data.Get(), TargetASC);
			}
		}
	}
}

void AC_TrapZone::OnLifeTimeExpired()
{
	if (!bTriggered)
	{
		DestroyTrap();
	}
}

void AC_TrapZone::DestroyTrap()
{
	GetWorldTimerManager().ClearTimer(LifeTimerHandle);
#if !UE_BUILD_SHIPPING
	FlushPersistentDebugLines(GetWorld());
#endif
	Destroy();
}
