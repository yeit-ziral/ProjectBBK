// Fill out your copyright notice in the Description page of Project Settings.

#include "C_FireZone.h"
#include "Components/SphereComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectTypes.h"
#include "../Monster/C_BaseMonster.h"

AC_FireZone::AC_FireZone()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionSphere->SetGenerateOverlapEvents(true);
	RootComponent = CollisionSphere;
}

void AC_FireZone::BeginPlay()
{
	Super::BeginPlay();
}

void AC_FireZone::Initialize(
	UAbilitySystemComponent* InInstigatorASC,
	AActor* InInstigatorActor,
	TSubclassOf<UGameplayEffect> InStatusEffectClass,
	TSubclassOf<UGameplayEffect> InDamageEffectClass,
	float InRadius,
	float InLifetime,
	float InDamageAmount,
	float InDotDuration)
{
	InstigatorASC = InInstigatorASC;
	InstigatorActor = InInstigatorActor;
	StatusEffectClass = InStatusEffectClass;
	DamageEffectClass = InDamageEffectClass;
	DamageAmount = InDamageAmount;
	DotDuration = InDotDuration;

	CollisionSphere->SetSphereRadius(InRadius);

	// 존 생성 시점에 이미 범위 안에 있는 몬스터에게 즉시 적용
	TArray<AActor*> OverlappingActors;
	CollisionSphere->GetOverlappingActors(OverlappingActors, AC_BaseMonster::StaticClass());
	for (AActor* Actor : OverlappingActors)
	{
		ApplyEffectToTarget(Actor);
	}

	// 이후 진입하는 몬스터를 위해 바인딩 (GetOverlappingActors 이후에 등록해 중복 적용 방지)
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AC_FireZone::OnBeginOverlap);

	// 수명 타이머
	GetWorldTimerManager().SetTimer(
		LifetimeHandle,
		this,
		&AC_FireZone::OnExpired,
		InLifetime,
		false
	);

	// BP에서 VFX 스케일 조정
	OnInitialized(InRadius);
}

void AC_FireZone::OnBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || !OtherActor->IsA(AC_BaseMonster::StaticClass()))
	{
		return;
	}

	ApplyEffectToTarget(OtherActor);
}

void AC_FireZone::ApplyEffectToTarget(AActor* TargetActor)
{
	if (!InstigatorASC.IsValid() || !InstigatorActor.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);

	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = InstigatorASC->MakeEffectContext();
	Context.AddInstigator(InstigatorActor.Get(), InstigatorActor.Get());

	// 1. 상태이상 GE 적용 (태그 + Cue)
	if (StatusEffectClass)
	{
		FGameplayEffectSpecHandle StatusSpec = InstigatorASC->MakeOutgoingSpec(
			StatusEffectClass,
			1.0f,
			Context
		);

		if (StatusSpec.IsValid())
		{
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*StatusSpec.Data, TargetASC);
		}
	}

	// 2. 데미지 GE 적용 (Set by Caller로 데미지 전달)
	if (DamageEffectClass)
	{
		FGameplayEffectSpecHandle DamageSpec = InstigatorASC->MakeOutgoingSpec(
			DamageEffectClass,
			1.0f,
			Context
		);

		if (DamageSpec.IsValid())
		{
			FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
			FGameplayTag DurationTag = FGameplayTag::RequestGameplayTag(FName("Data.Duration"));
			DamageSpec.Data->SetSetByCallerMagnitude(DamageTag, DamageAmount);
			DamageSpec.Data->SetSetByCallerMagnitude(DurationTag, DotDuration);

			InstigatorASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data, TargetASC);

			UE_LOG(LogTemp, Log, TEXT("[FireZone] Applied %s to %s (Damage: %.1f)"),
				*DamageEffectClass->GetName(), *TargetActor->GetName(), DamageAmount);
		}
	}
}

void AC_FireZone::OnExpired()
{
	UE_LOG(LogTemp, Log, TEXT("[FireZone] Zone expired, destroying."));
	Destroy();
}
