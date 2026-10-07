// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EliteSummonCircle.h"
#include "../C_BaseMonster.h"
#include "../../LevelSystem/C_BBKGameMode.h"
#include "Components/DecalComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AC_EliteSummonCircle::AC_EliteSummonCircle()
{
	// 솟아오르기·마법진 페이드가 진행 중일 때만 Tick을 켠다
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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
		TEXT("/Game/Monster/Decal/Material/Decal/M_EliteSummonCircle.M_EliteSummonCircle"));
	if (CircleMaterialFinder.Succeeded())
	{
		summonCircleMaterial = CircleMaterialFinder.Object;
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

	if (summonCircleMaterial)
	{
		circleMID = UMaterialInstanceDynamic::Create(summonCircleMaterial, this);
		circleMID->SetVectorParameterValue(colorParameterName, summonCircleColor);
		circleMID->SetScalarParameterValue(intensityParameterName, circleIntensity);
		circleDecal->SetDecalMaterial(circleMID);
	}
	SetCircleOpacity(0.f);

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

	// 마법진이 서서히 나타남
	circleDecal->SetVisibility(true);
	circleFadeDirection = 1;
	SetActorTickEnabled(true);

	GetWorldTimerManager().SetTimer(summonTimer, this, &AC_EliteSummonCircle::BeginRise, FMath::Max(summonDelay, 0.01f), false);
}

void AC_EliteSummonCircle::BeginRise()
{
	FVector SpawnLocation = FindGroundLocation();

	// 캡슐 바닥이 지면에 닿도록 반높이만큼 올림
	float HalfHeight = 0.f;
	if (const ACharacter* CDO = summonClass->GetDefaultObject<ACharacter>())
	{
		HalfHeight = CDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		SpawnLocation.Z += HalfHeight;
	}

	// 플레이어를 바라보며 등장
	FRotator SpawnRotation = GetActorRotation();
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		SpawnRotation = FRotator(0.f, (Player->GetActorLocation() - SpawnLocation).Rotation().Yaw, 0.f);
	}

	// 도착 위치에서 스폰한다 — BeginPlay가 이 위치를 배회 기준점(idleHomeLocation) 등으로 기록하므로.
	// level은 BeginPlay의 DataComponent 초기화에서 스탯 계산에 쓰이므로 FinishSpawning 전에 주입
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	AC_BaseMonster* Summoned = GetWorld()->SpawnActorDeferred<AC_BaseMonster>(
		summonClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Summoned)
	{
		// 스폰 실패 시 예약해 둔 카운트를 되돌려 포탈이 영영 안 열리는 상황 방지
		if (AC_BBKGameMode* GM = GetWorld()->GetAuthGameMode<AC_BBKGameMode>())
		{
			GM->NotifyMonsterDead();
		}
		StartCircleFadeOut();
		return;
	}

	Summoned->level = summonLevel;
	Summoned->FinishSpawning(SpawnTransform);

	// ── 솟아오르는 동안은 "아직 없는 몬스터"로 취급: 맞지도, 때리지도, 움직이지도 않는다 ──
	Summoned->SetActorEnableCollision(false);
	Summoned->SetActorTickEnabled(false);   // 공격 쿨타임 시계·디버그 자동공격 정지 (메시 애니메이션은 계속 재생)

	if (UCharacterMovementComponent* Movement = Summoned->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_None);   // 바닥 아래에서 중력·밀어내기가 작동하지 않게
	}

	if (AAIController* AI = Cast<AAIController>(Summoned->GetController()))
	{
		if (UBrainComponent* Brain = AI->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Summoning"));
		}
	}

	// HP 바가 바닥을 뚫고 먼저 보이지 않게
	TArray<UWidgetComponent*> Widgets;
	Summoned->GetComponents<UWidgetComponent>(Widgets);
	for (UWidgetComponent* Widget : Widgets)
	{
		Widget->SetHiddenInGame(true);
	}

	// 몸 전체가 바닥 아래로 들어가도록 묻는다
	riseTargetLocation = SpawnLocation;
	riseDepth = HalfHeight * 2.f + riseExtraDepth;
	riseElapsed = 0.f;
	risingMonster = Summoned;
	bRising = true;

	Summoned->SetActorLocation(riseTargetLocation - FVector(0.f, 0.f, riseDepth), false, nullptr, ETeleportType::TeleportPhysics);
	SetActorTickEnabled(true);
}

void AC_EliteSummonCircle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 마법진 페이드
	if (circleFadeDirection != 0)
	{
		const float Step = circleFadeTime > KINDA_SMALL_NUMBER ? DeltaSeconds / circleFadeTime : 1.f;
		const float NewOpacity = FMath::Clamp(circleOpacity + Step * circleFadeDirection, 0.f, 1.f);
		SetCircleOpacity(NewOpacity);

		if (NewOpacity >= 1.f || NewOpacity <= 0.f)
		{
			if (circleFadeDirection < 0) circleDecal->SetVisibility(false);
			circleFadeDirection = 0;
		}
	}

	// 몬스터 솟아오르기
	if (bRising)
	{
		AC_BaseMonster* Monster = risingMonster.Get();
		if (!Monster)
		{
			// 올라오는 도중 사라짐(레벨 정리 등)
			bRising = false;
			StartCircleFadeOut();
		}
		else
		{
			riseElapsed += DeltaSeconds;
			const float Alpha = FMath::Clamp(riseElapsed / FMath::Max(riseDuration, 0.1f), 0.f, 1.f);

			// 처음엔 천천히, 끝에서 부드럽게 멈춤
			const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
			Monster->SetActorLocation(riseTargetLocation - FVector(0.f, 0.f, riseDepth * (1.f - Eased)),
				false, nullptr, ETeleportType::TeleportPhysics);

			if (Alpha >= 1.f)
			{
				FinishRise();
			}
		}
	}

	if (!bRising && circleFadeDirection == 0)
	{
		SetActorTickEnabled(false);
	}
}

void AC_EliteSummonCircle::FinishRise()
{
	bRising = false;

	if (AC_BaseMonster* Monster = risingMonster.Get())
	{
		Monster->SetActorLocation(riseTargetLocation, false, nullptr, ETeleportType::TeleportPhysics);

		// ── 여기서부터 진짜 몬스터: 콜리전 → 이동 → Tick → AI 순서로 켠다 ──
		Monster->SetActorEnableCollision(true);

		if (UCharacterMovementComponent* Movement = Monster->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}

		Monster->SetActorTickEnabled(true);

		TArray<UWidgetComponent*> Widgets;
		Monster->GetComponents<UWidgetComponent>(Widgets);
		for (UWidgetComponent* Widget : Widgets)
		{
			Widget->SetHiddenInGame(false);
		}

		if (AAIController* AI = Cast<AAIController>(Monster->GetController()))
		{
			if (UBrainComponent* Brain = AI->GetBrainComponent())
			{
				Brain->RestartLogic();
			}
		}

		OnEliteSummoned.Broadcast(Monster);
	}

	risingMonster.Reset();

	if (circleLingerTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(hideTimer, this, &AC_EliteSummonCircle::StartCircleFadeOut, circleLingerTime, false);
	}
	else
	{
		StartCircleFadeOut();
	}
}

void AC_EliteSummonCircle::StartCircleFadeOut()
{
	circleFadeDirection = -1;
	SetActorTickEnabled(true);
}

void AC_EliteSummonCircle::SetCircleOpacity(float Opacity)
{
	circleOpacity = Opacity;
	if (circleMID)
	{
		circleMID->SetScalarParameterValue(opacityParameterName, Opacity);
	}
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
