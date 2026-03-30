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

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AC_FireZone::OnBeginOverlap);
}

void AC_FireZone::Initialize(
	UAbilitySystemComponent* InInstigatorASC,
	AActor* InInstigatorActor,
	TSubclassOf<UGameplayEffect> InEffectClass,
	float InRadius,
	float InLifetime,
	float InDamageAmount)
{
	InstigatorASC = InInstigatorASC;
	InstigatorActor = InInstigatorActor;
	EffectClass = InEffectClass;
	DamageAmount = InDamageAmount;

	CollisionSphere->SetSphereRadius(InRadius);

	// 존 생성 시점에 이미 범위 안에 있는 몬스터에게도 즉시 적용
	TArray<AActor*> OverlappingActors;
	CollisionSphere->GetOverlappingActors(OverlappingActors, AC_BaseMonster::StaticClass());
	for (AActor* Actor : OverlappingActors)
	{
		ApplyEffectToTarget(Actor);
	}

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
	if (!EffectClass || !InstigatorASC.IsValid() || !InstigatorActor.IsValid())
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

	FGameplayEffectSpecHandle Spec = InstigatorASC->MakeOutgoingSpec(
		EffectClass,
		1.0f,
		Context
	);

	if (Spec.IsValid())
	{
		// Set by Caller로 데미지 전달
		FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
		Spec.Data->SetSetByCallerMagnitude(DamageTag, DamageAmount);

		InstigatorASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);

		UE_LOG(LogTemp, Log, TEXT("[FireZone] Applied %s to %s (Damage: %.1f)"),
			*EffectClass->GetName(), *TargetActor->GetName(), DamageAmount);
	}
}

void AC_FireZone::OnExpired()
{
	UE_LOG(LogTemp, Log, TEXT("[FireZone] Zone expired, destroying."));
	Destroy();
}
