// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossBeam.h"
#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

// Sets default values
AC_BossBeam::AC_BossBeam()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    root = CreateDefaultSubobject<USceneComponent>("Root");
    SetRootComponent(root);

    previewPlane = CreateDefaultSubobject<UStaticMeshComponent>("PreviewPlane");
    previewPlane->SetupAttachment(root);

    beam = CreateDefaultSubobject<UNiagaraComponent>("Beam");
    beam->SetupAttachment(root);
}

// Called when the game starts or when spawned
void AC_BossBeam::BeginPlay()
{
	Super::BeginPlay();

    beam->Deactivate();

    // 일정 시간 뒤 실제 빔
    FTimerHandle handle;
    GetWorld()->GetTimerManager().SetTimer(handle, this, &AC_BossBeam::SwitchToBeam, previewTime, false);

    SetLifeSpan(lifeTime);
}

// Called every frame
void AC_BossBeam::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!ownerBoss) return;

    FVector bossLocation = ownerBoss->GetActorLocation();

    // 현재 빔이 바라보는 방향
    FVector forward = GetActorForwardVector();

    // 보스 + 앞방향 * 거리
    FVector newLocation = bossLocation + forward * distanceFromBoss;

    SetActorLocation(newLocation);

    if (!bIsPreview)
    {
        FRotator rot = GetActorRotation();
        rot.Yaw += rotateSpeed * DeltaTime;
        SetActorRotation(rot);
    }
}

void AC_BossBeam::SwitchToBeam()
{
    bIsPreview = false;

    previewPlane->SetVisibility(false);
    beam->Activate();
}

