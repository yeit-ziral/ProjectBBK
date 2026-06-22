// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EliteSwampZone.h"
#include "Components/DecalComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "../../GAS/Attributes/C_ChracterAttributeSetBase.h"

AC_EliteSwampZone::AC_EliteSwampZone()
{
	PrimaryActorTick.bCanEverTick = false;

	// 회전 없는 씬 루트 → 데칼은 아래로 투영, 안개는 위로 피어오르도록 분리
	sceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = sceneRoot;

	decalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
	decalComponent->SetupAttachment(sceneRoot);
	// 데칼은 로컬 -X 방향으로 투영 → 바닥 수직 투영을 위해 Pitch -90
	decalComponent->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	decalComponent->DecalSize = FVector(200.f, 300.f, 300.f);

	mistComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MistComponent"));
	mistComponent->SetupAttachment(sceneRoot);
	mistComponent->bAutoActivate = false;
}

void AC_EliteSwampZone::BeginPlay()
{
	Super::BeginPlay();

	if (decalMaterial)
		decalComponent->SetDecalMaterial(decalMaterial);
}

void AC_EliteSwampZone::InitSwampZone(UAbilitySystemComponent* InOwnerASC, float InRadius, float InHealthPercent, float InTickRate, float InDuration)
{
	ownerASC      = InOwnerASC;
	radius        = InRadius;
	healthPercent = InHealthPercent;

	// 데칼 크기를 반경에 맞춤 (Y/Z half-extent = radius, X = 투영 깊이)
	decalComponent->DecalSize = FVector(200.f, radius, radius);

	// 안개 재생 — 이 aura는 스폰 반경이 아닌 속도/궤도로 모양이 고정되어
	// 단일 인스턴스로는 장판 전체를 못 덮는다. 자연 크기 인스턴스를
	// 디스크 전역(반경 × mistCoverage)에 흩뿌려 부착해 균일 커버를 만든다.
	if (mistEffect && mistComponent)
	{
		const int32 count    = FMath::Max(1, mistInstanceCount);
		const float coverage = radius * mistCoverage;
		FRandomStream stream(GetUniqueID());
		const float goldenAngle = 2.39996323f;        // ≈137.5° (해바라기 분포)
		const float jitter      = coverage * 0.06f;   // 격자감 제거용 미세 지터

		// 해바라기(골든앵글) 분포로 디스크를 시드와 무관하게 균일 충진 + 지터
		auto SetupInstance = [&](UNiagaraComponent* nc, int32 idx)
		{
			const float rr  = coverage * FMath::Sqrt((idx + 0.5f) / count);
			const float ang = idx * goldenAngle + stream.FRandRange(-0.4f, 0.4f);
			const float x   = FMath::Cos(ang) * rr + stream.FRandRange(-jitter, jitter);
			const float y   = FMath::Sin(ang) * rr + stream.FRandRange(-jitter, jitter);
			nc->SetRelativeScale3D(FVector(mistScaleMultiplier));
			nc->SetRelativeLocation(FVector(x, y, 0.f));
			nc->SetAsset(mistEffect);
			nc->Activate(true);
		};

		// 인스턴스 0 = 생성자에서 만든 mistComponent
		SetupInstance(mistComponent, 0);

		// 나머지는 런타임 스폰 후 sceneRoot에 부착 (몬스터 따라 이동)
		for (int32 i = 1; i < count; ++i)
		{
			UNiagaraComponent* nc = NewObject<UNiagaraComponent>(this);
			if (!nc) continue;
			nc->SetupAttachment(sceneRoot);
			nc->bAutoActivate = false;
			nc->RegisterComponent();
			SetupInstance(nc, i);
			mistInstances.Add(nc);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(
		damageTimerHandle, this, &AC_EliteSwampZone::TickDamage, InTickRate, true, InTickRate);

	GetWorld()->GetTimerManager().SetTimer(
		lifetimeTimerHandle, this, &AC_EliteSwampZone::DestroyZone, InDuration, false);
}

void AC_EliteSwampZone::StopZone()
{
	if (UWorld* world = GetWorld())
	{
		world->GetTimerManager().ClearTimer(damageTimerHandle);
		world->GetTimerManager().ClearTimer(lifetimeTimerHandle);
	}
}

void AC_EliteSwampZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopZone();
	Super::EndPlay(EndPlayReason);
}

void AC_EliteSwampZone::TickDamage()
{
	if (!ownerASC.IsValid() || !damageEffectClass) return;

	APawn* player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!player) return;

	// 장판은 몬스터에 부착되어 따라다니므로, 현재 액터 위치를 중심으로 판정 (높이 무시)
	FVector delta = player->GetActorLocation() - GetActorLocation();
	delta.Z = 0.f;
	if (delta.Size() > radius) return;   // 반경 "안"에 있을 때만 데미지

	IAbilitySystemInterface* ascInterface = Cast<IAbilitySystemInterface>(player);
	if (!ascInterface) return;

	UAbilitySystemComponent* targetASC = ascInterface->GetAbilitySystemComponent();
	if (!targetASC) return;

	const float maxHealth = targetASC->GetNumericAttribute(
		UC_ChracterAttributeSetBase::GetmaxHealthAttribute());
	if (maxHealth <= 0.f) return;

	const float damage = maxHealth * healthPercent;

	FGameplayEffectContextHandle context = ownerASC->MakeEffectContext();
	context.AddInstigator(GetOwner(), GetOwner());

	FGameplayEffectSpecHandle spec = ownerASC->MakeOutgoingSpec(damageEffectClass, 1.f, context);
	if (!spec.IsValid()) return;

	spec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag("Data.Damage"), damage);

	ownerASC->ApplyGameplayEffectSpecToTarget(*spec.Data.Get(), targetASC);
}

void AC_EliteSwampZone::DestroyZone()
{
	StopZone();
	Destroy();
}
