// Fill out your copyright notice in the Description page of Project Settings.

#include "C_ExpOrb.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"

AC_ExpOrb::AC_ExpOrb()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);

	NiagaraEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraEffect"));
	NiagaraEffect->SetupAttachment(CollisionSphere);
}

void AC_ExpOrb::BeginPlay()
{
	Super::BeginPlay();
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AC_ExpOrb::OnOrbOverlap);
}

void AC_ExpOrb::InitOrb(float InExpAmount)
{
	ExpAmount = InExpAmount;
}

void AC_ExpOrb::OnOrbOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bConsumed) return;

	AC_BasePlayerCharactor* Player = Cast<AC_BasePlayerCharactor>(OtherActor);
	if (!Player) return;

	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (!ASC || !GE_GainExperience) return;

	bConsumed = true;

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GE_GainExperience, 1.f, ASC->MakeEffectContext());
	SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Exp")), ExpAmount);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	Destroy();
}
