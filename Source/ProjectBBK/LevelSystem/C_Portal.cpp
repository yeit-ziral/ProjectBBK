// Fill out your copyright notice in the Description page of Project Settings.

#include "C_Portal.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"

AC_Portal::AC_Portal()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	PortalCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("PortalCollision"));
	PortalCollision->SetupAttachment(RootComp);
	PortalCollision->SetBoxExtent(FVector(20.f, 300.f, 400.f));
	PortalCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalCollision->SetCollisionObjectType(ECC_WorldDynamic);
	PortalCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PortalCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PortalEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalEffect"));
	PortalEffect->SetupAttachment(RootComp);
	PortalEffect->bAutoActivate = false;
}

void AC_Portal::BeginPlay()
{
	Super::BeginPlay();

	PortalCollision->OnComponentBeginOverlap.AddDynamic(this, &AC_Portal::OnPortalOverlap);
	DeactivatePortal();
}

void AC_Portal::ActivatePortal()
{
	bIsActivated = true;
	PortalCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PortalEffect->Activate(true);
}

void AC_Portal::DeactivatePortal()
{
	bIsActivated = false;
	PortalCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalEffect->Deactivate();
}

void AC_Portal::OnPortalOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsActivated) return;

	if (Cast<AC_BasePlayerCharactor>(OtherActor))
	{
		OnPortalEntered.Broadcast(this);
	}
}
