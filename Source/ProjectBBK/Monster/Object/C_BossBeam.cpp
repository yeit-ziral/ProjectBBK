// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossBeam.h"
#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

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
    beam->SetAutoActivate(false);
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

void AC_BossBeam::InitBeam(UAbilitySystemComponent* InInstigatorASC,
                            TSubclassOf<UGameplayEffect> InDamageGEClass,
                            float InDamageValue,
                            float InBeamRange,
                            float InDamageTickRate)
{
    instigatorASC    = InInstigatorASC;
    damageGEClass    = InDamageGEClass;
    damageValue      = InDamageValue;
    beamRange        = InBeamRange;
    damageTickRate   = InDamageTickRate;
}

void AC_BossBeam::SwitchToBeam()
{
    // 내부 타이머보다 외부에서 먼저 호출된 경우 타이머 정리
    GetWorld()->GetTimerManager().ClearTimer(previewTimerHandle);

    bIsPreview = false;
    previewPlane->SetVisibility(false);
    beam->Activate();

    // 빔 활성화 시점부터 데미지 틱 시작
    if (damageGEClass && instigatorASC.IsValid())
    {
        GetWorld()->GetTimerManager().SetTimer(
            damageTickHandle, this, &AC_BossBeam::ApplyBeamDamage, damageTickRate, true);
    }
}

void AC_BossBeam::ApplyBeamDamage()
{
    if (!ownerBoss || !instigatorASC.IsValid() || !damageGEClass) return;

    APawn* player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!player) return;

    IAbilitySystemInterface* ascInterface = Cast<IAbilitySystemInterface>(player);
    if (!ascInterface) return;

    UAbilitySystemComponent* targetASC = ascInterface->GetAbilitySystemComponent();
    if (!targetASC) return;

    const FVector bossLoc  = ownerBoss->GetActorLocation();
    const FVector toPlayer = player->GetActorLocation() - bossLoc;
    const float   dist2D   = FVector(toPlayer.X, toPlayer.Y, 0.f).Size();

    // 사거리 밖이면 무시
    if (dist2D > beamRange || dist2D < distanceFromBoss) return;

    // 빔 정방향과 플레이어 방향의 각도 체크 (수평 기준)
    const FVector forward2D   = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
    const FVector toPlayer2D  = FVector(toPlayer.X, toPlayer.Y, 0.f).GetSafeNormal();
    const float   dot         = FVector::DotProduct(forward2D, toPlayer2D);

    // cos(20°) ≈ 0.94 — 빔 폭 허용 각도
    if (dot < 0.94f) return;

    FGameplayEffectContextHandle context = instigatorASC->MakeEffectContext();
    context.AddInstigator(ownerBoss, ownerBoss);

    FGameplayEffectSpecHandle spec = instigatorASC->MakeOutgoingSpec(damageGEClass, 1.f, context);
    if (!spec.IsValid()) return;

    spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), damageValue);
    instigatorASC->ApplyGameplayEffectSpecToTarget(*spec.Data.Get(), targetASC);
}

void AC_BossBeam::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* world = GetWorld())
    {
        world->GetTimerManager().ClearTimer(previewTimerHandle);
        world->GetTimerManager().ClearTimer(damageTickHandle);
    }
    Super::EndPlay(EndPlayReason);
}

