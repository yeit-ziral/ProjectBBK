// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossGhostClone.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "../../PlayerCharacter/C_BasePlayerCharactor.h"
#include "../M_Gas/GA/C_BossGridLaserPatternGA.h"

AC_BossGhostClone::AC_BossGhostClone()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* rootComp = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(rootComp);

	ghostMesh = CreateDefaultSubobject<USkeletalMeshComponent>("GhostMesh");
	ghostMesh->SetupAttachment(rootComp);
	// UE 캐릭터 메시는 로컬 +Y를 바라보므로 -90 Yaw로 액터 forward(+X)에 정렬
	// 메시 임포트 방향이 다를 경우 BPC_BossGhostClone에서 GhostMesh Rotation을 조정
	ghostMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	markMesh = CreateDefaultSubobject<UStaticMeshComponent>("MarkMesh");
	markMesh->SetupAttachment(rootComp);
	markMesh->SetRelativeLocation(FVector(0.f, 0.f, 5.f));
	markMesh->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	markMesh->SetCastShadow(false);
	markMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	markMesh->SetVisibility(false);

	hitDetectSphere = CreateDefaultSubobject<USphereComponent>("HitDetectSphere");
	hitDetectSphere->SetupAttachment(rootComp);
	hitDetectSphere->SetSphereRadius(80.f);
	hitDetectSphere->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	hitDetectSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	hitDetectSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	hitDetectSphere->OnComponentBeginOverlap.AddDynamic(this, &AC_BossGhostClone::OnHitSphereBeginOverlap);
}

void AC_BossGhostClone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsMarked)
	{
		const FVector center = GetActorLocation() + FVector(0.f, 0.f, 120.f);
		//DrawDebugSphere(GetWorld(), center, 70.f, 16, FColor::Yellow, false, -1.f, 0, 5.f);
		//DrawDebugSphere(GetWorld(), center, 80.f, 16, FColor::Orange, false, -1.f, 0, 3.f);
	}
}

void AC_BossGhostClone::Initialize(UC_BossGridLaserPatternGA* InOwningGA, const FVector& InLaserDirection)
{
	owningGA = InOwningGA;
	laserDirection = InLaserDirection;
	SetActorRotation(InLaserDirection.Rotation());
}

void AC_BossGhostClone::PlayCharge(UAnimMontage* ChargeMontage, float Duration)
{
	if (!ChargeMontage || !ghostMesh) return;

	// 모션 길이를 chargeDuration에 맞게 늘려서 슬로우 모션처럼 재생
	const float montageLength = ChargeMontage->GetPlayLength();
	if (montageLength > 0.f && Duration > 0.f)
		ghostMesh->GlobalAnimRateScale = montageLength / Duration;

	ghostMesh->PlayAnimation(ChargeMontage, false);
}

void AC_BossGhostClone::PlayFire(UAnimMontage* FireMontage)
{
	if (!FireMontage || !ghostMesh) return;

	ghostMesh->GlobalAnimRateScale = 1.f;
	ghostMesh->PlayAnimation(FireMontage, true);
}

void AC_BossGhostClone::SetMarked(bool bMarked)
{
	bIsMarked = bMarked;

	SetActorTickEnabled(bMarked);

	markMesh->SetVisibility(bMarked);

	hitDetectSphere->SetCollisionEnabled(
		bMarked ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void AC_BossGhostClone::OnHitSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsMarked || bCancelled) return;

	// 플레이어 공격 콜라이더 감지
	// ANS_Collider 프로파일에 따라 조건 정밀화 필요 — 현재는 플레이어 근접 기준
	if (!OtherActor || !OtherActor->IsA<AC_BasePlayerCharactor>()) return;

	if (!owningGA.IsValid()) return;

	bCancelled = true;
	owningGA->HandleMarkedGhostDestroyed(this);
	Destroy();
}
