// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossStorm.h"

#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AC_BossStorm::AC_BossStorm()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    PrimaryActorTick.bCanEverTick = false;

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

// Called when the game starts or when spawned
void AC_BossStorm::BeginPlay()
{
	Super::BeginPlay();

    SetLifeSpan(stormLifeTime);
}

// Called every frame
void AC_BossStorm::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

