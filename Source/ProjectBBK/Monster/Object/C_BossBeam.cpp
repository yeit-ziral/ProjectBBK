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

    GetWorld()->GetTimerManager().SetTimer(previewTimerHandle, this, &AC_BossBeam::SwitchToBeam, previewTime, false);
}

// Called every frame
void AC_BossBeam::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!ownerBoss) return;

    FVector bossLocation = ownerBoss->GetActorLocation();

    // ���� ���� �ٶ󺸴� ����
    FVector forward = GetActorForwardVector();

    // ���� + �չ��� * �Ÿ�
    FVector newLocation = bossLocation + forward * distanceFromBoss;

    SetActorLocation(newLocation);

    if (!bIsPreview)
    {
        FRotator rot = GetActorRotation();
        rot.Yaw += rotateSpeed * DeltaTime;
        SetActorRotation(rot);
    }
}

void AC_BossBeam::DisableAutoSwitch()
{
    GetWorld()->GetTimerManager().ClearTimer(previewTimerHandle);
}

void AC_BossBeam::SwitchToBeam()
{
    // 내부 타이머보다 외부에서 먼저 호출된 경우 타이머 정리
    GetWorld()->GetTimerManager().ClearTimer(previewTimerHandle);

    bIsPreview = false;
    previewPlane->SetVisibility(false);
    beam->Activate();
}

