// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EliteSummonCircle.h"
#include "../C_BaseMonster.h"
#include "../../LevelSystem/C_BBKGameMode.h"
#include "Components/DecalComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AC_EliteSummonCircle::AC_EliteSummonCircle()
{
	PrimaryActorTick.bCanEverTick = false;

	root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = root;

	// 데칼은 로컬 -X로 투영 → Pitch -90으로 바닥을 향하게 (Debugging Checklist #21)
	circleDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("CircleDecal"));
	circleDecal->SetupAttachment(root);
	circleDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	circleDecal->SetVisibility(false);

	editorIcon = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorIcon"));
	editorIcon->SetupAttachment(root);
	editorIcon->bIsEditorOnly = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CircleMaterialFinder(
		TEXT("/Game/Monster/Decal/Material/Decal/MagicCircleFiredecal.MagicCircleFiredecal"));
	if (CircleMaterialFinder.Succeeded())
	{
		circleMaterial = CircleMaterialFinder.Object;
	}

	static ConstructorHelpers::FClassFinder<AC_BaseMonster> EliteClassFinder(
		TEXT("/Game/Monster/MonsterBP/Elite/BPC_EliteMonster"));
	if (EliteClassFinder.Succeeded())
	{
		summonClass = EliteClassFinder.Class;
	}
}

void AC_EliteSummonCircle::BeginPlay()
{
	Super::BeginPlay();

	// 마법진을 바닥 높이로 내림 — 배치 위치가 공중이어도 바닥에 투영되도록
	circleDecal->SetWorldLocation(FindGroundLocation());
	circleDecal->DecalSize = FVector(decalDepth, circleRadius, circleRadius);

	if (circleMaterial)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(circleMaterial, this);
		MID->SetVectorParameterValue(colorParameterName, circleColor);
		circleDecal->SetDecalMaterial(MID);
	}

	if (AC_BBKGameMode* GM = GetWorld()->GetAuthGameMode<AC_BBKGameMode>())
	{
		GM->OnAllMonstersDefeated.AddUObject(this, &AC_EliteSummonCircle::HandleAllMonstersDefeated);
	}
}

void AC_EliteSummonCircle::HandleAllMonstersDefeated()
{
	if (bTriggered || !summonClass) return;
	bTriggered = true;

	// 엘리트를 미리 집계에 넣어 이번 전멸 시점에 포탈이 열리지 않게 함 (브로드캐스트 직후 GameMode가 카운트 재확인)
	if (AC_BBKGameMode* GM = GetWorld()->GetAuthGameMode<AC_BBKGameMode>())
	{
		GM->RegisterPendingMonster();
	}

	circleDecal->SetVisibility(true);
	GetWorldTimerManager().SetTimer(summonTimer, this, &AC_EliteSummonCircle::SpawnSummon, FMath::Max(summonDelay, 0.01f), false);
}

void AC_EliteSummonCircle::SpawnSummon()
{
	FVector SpawnLocation = FindGroundLocation();

	// 캡슐 바닥이 지면에 닿도록 반높이만큼 올림
	if (const ACharacter* CDO = summonClass->GetDefaultObject<ACharacter>())
	{
		SpawnLocation.Z += CDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}

	// 플레이어를 바라보며 등장
	FRotator SpawnRotation = GetActorRotation();
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		SpawnRotation = FRotator(0.f, (Player->GetActorLocation() - SpawnLocation).Rotation().Yaw, 0.f);
	}

	// level은 BeginPlay의 DataComponent 초기화에서 스탯 계산에 쓰이므로 FinishSpawning 전에 주입
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	AC_BaseMonster* Summoned = GetWorld()->SpawnActorDeferred<AC_BaseMonster>(
		summonClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (Summoned)
	{
		Summoned->level = summonLevel;
		Summoned->FinishSpawning(SpawnTransform);
		OnEliteSummoned.Broadcast(Summoned);
	}
	else if (AC_BBKGameMode* GM = GetWorld()->GetAuthGameMode<AC_BBKGameMode>())
	{
		// 스폰 실패 시 예약해 둔 카운트를 되돌려 포탈이 영영 안 열리는 상황 방지
		GM->NotifyMonsterDead();
	}

	if (circleLingerTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(hideTimer, this, &AC_EliteSummonCircle::HideCircle, circleLingerTime, false);
	}
	else
	{
		HideCircle();
	}
}

void AC_EliteSummonCircle::HideCircle()
{
	circleDecal->SetVisibility(false);
}

FVector AC_EliteSummonCircle::FindGroundLocation() const
{
	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 500.f);
	const FVector End = GetActorLocation() - FVector(0.f, 0.f, 5000.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EliteSummonGround), false, this);
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Params.AddIgnoredActor(Player);
	}

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.bBlockingHit)
	{
		return Hit.ImpactPoint;
	}

	// 바닥을 못 찾으면 배치 위치 그대로 (Debugging Checklist #22 — ImpactPoint(0,0,0) 사용 방지)
	return GetActorLocation();
}
