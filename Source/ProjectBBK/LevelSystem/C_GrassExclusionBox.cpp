// Fill out your copyright notice in the Description page of Project Settings.

#include "C_GrassExclusionBox.h"
#include "Components/BoxComponent.h"
#include "LandscapeProxy.h"

AC_GrassExclusionBox::AC_GrassExclusionBox()
{
	PrimaryActorTick.bCanEverTick = false;

	exclusionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ExclusionBox"));
	SetRootComponent(exclusionBox);
	exclusionBox->SetBoxExtent(FVector(500.f, 500.f, 2000.f));
	exclusionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	exclusionBox->SetCanEverAffectNavigation(false);
	exclusionBox->SetHiddenInGame(true);
	exclusionBox->ShapeColor = FColor(80, 220, 80);
}

void AC_GrassExclusionBox::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	RegisterExclusion();
}

void AC_GrassExclusionBox::PostUnregisterAllComponents()
{
	UnregisterExclusion();
	Super::PostUnregisterAllComponents();
}

#if WITH_EDITOR
void AC_GrassExclusionBox::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (bFinished)
		RegisterExclusion();
}

void AC_GrassExclusionBox::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RegisterExclusion();
}
#endif

void AC_GrassExclusionBox::RegisterExclusion()
{
	if (!exclusionBox || HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) return;

	// 같은 Owner로 다시 Add하면 TMap 값이 교체된다 — 이동·크기 변경 시 그대로 갱신
	ALandscapeProxy::AddExclusionBox(FWeakObjectPtr(this), exclusionBox->Bounds.GetBox());
	bRegistered = true;
}

void AC_GrassExclusionBox::UnregisterExclusion()
{
	if (!bRegistered) return;

	ALandscapeProxy::RemoveExclusionBox(FWeakObjectPtr(this));
	bRegistered = false;
}
