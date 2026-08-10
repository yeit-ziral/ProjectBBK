// Fill out your copyright notice in the Description page of Project Settings.

#include "C_HealZone.h"
#include "Components/SphereComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectTypes.h"

AC_HealZone::AC_HealZone()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionSphere->SetGenerateOverlapEvents(true);
	RootComponent = CollisionSphere;
}

void AC_HealZone::BeginPlay()
{
	Super::BeginPlay();
}

void AC_HealZone::Initialize(
	UAbilitySystemComponent* InInstigatorASC,
	AActor* InInstigatorActor,
	TSubclassOf<UGameplayEffect> InTickEffectClass,
	float InRadius,
	float InZoneLifetime,
	float InTickInterval,
	float InHealPerTick)
{
	InstigatorASC = InInstigatorASC;
	InstigatorActor = InInstigatorActor;
	TickEffectClass = InTickEffectClass;
	TickInterval = InTickInterval;
	HealPerTick = InHealPerTick;

	CollisionSphere->SetSphereRadius(InRadius);

	// 스폰 시점에 이미 반경 안에 있으면(발밑 스폰이라 항상 해당) 즉시 회복 시작
	TArray<AActor*> OverlappingActors;
	CollisionSphere->GetOverlappingActors(OverlappingActors);
	if (InstigatorActor.IsValid() && OverlappingActors.Contains(InstigatorActor.Get()))
	{
		StartHealing();
	}

	// GetOverlappingActors 이후에 바인딩해 즉시 적용분과 중복되지 않도록 함
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AC_HealZone::OnBeginOverlap);
	CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &AC_HealZone::OnEndOverlap);

	// 존 액터 자체의 수명 — 회복 지속시간과는 무관 (회복은 체류 여부로만 결정)
	GetWorldTimerManager().SetTimer(
		LifetimeHandle,
		this,
		&AC_HealZone::OnExpired,
		InZoneLifetime,
		false
	);

	OnInitialized(InRadius);
}

void AC_HealZone::OnBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor != InstigatorActor.Get())
	{
		return;
	}

	StartHealing();
}

void AC_HealZone::OnEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!OtherActor || OtherActor != InstigatorActor.Get())
	{
		return;
	}

	StopHealing();
}

void AC_HealZone::StartHealing()
{
	if (GetWorldTimerManager().IsTimerActive(HealTickHandle))
	{
		return;   // 이미 회복 중 — 중복 타이머 방지
	}

	ApplyHealTick();   // 진입 즉시 1틱 적용

	GetWorldTimerManager().SetTimer(
		HealTickHandle,
		this,
		&AC_HealZone::ApplyHealTick,
		TickInterval,
		true
	);
}

void AC_HealZone::StopHealing()
{
	GetWorldTimerManager().ClearTimer(HealTickHandle);
}

void AC_HealZone::ApplyHealTick()
{
	if (!InstigatorASC.IsValid() || !TickEffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle Context = InstigatorASC->MakeEffectContext();
	Context.AddInstigator(InstigatorActor.Get(), InstigatorActor.Get());

	FGameplayEffectSpecHandle Spec = InstigatorASC->MakeOutgoingSpec(TickEffectClass, 1.0f, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	FGameplayTag HealTag = FGameplayTag::RequestGameplayTag(FName("Data.Heal"));
	Spec.Data->SetSetByCallerMagnitude(HealTag, HealPerTick);

	// 대상(InstigatorActor)이 곧 InstigatorASC의 소유자 — 자기 자신에게 적용
	InstigatorASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
}

void AC_HealZone::OnExpired()
{
	StopHealing();
	Destroy();
}
