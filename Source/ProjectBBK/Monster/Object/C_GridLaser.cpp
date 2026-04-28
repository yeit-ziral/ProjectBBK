// Fill out your copyright notice in the Description page of Project Settings.


#include "C_GridLaser.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "../M_Gas/C_MonsterASC.h"

AC_GridLaser::AC_GridLaser()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* rootComp = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(rootComp);
	root = rootComp;

	previewPlane = CreateDefaultSubobject<UStaticMeshComponent>("PreviewPlane");
	previewPlane->SetupAttachment(rootComp);
	previewPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	beamFX = CreateDefaultSubobject<UNiagaraComponent>("BeamFX");
	beamFX->SetupAttachment(rootComp);
	beamFX->bAutoActivate = false;

	damageBox = CreateDefaultSubobject<UBoxComponent>("DamageBox");
	damageBox->SetupAttachment(rootComp);
	damageBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	damageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	damageBox->OnComponentBeginOverlap.AddDynamic(this, &AC_GridLaser::OnHitBoxBeginOverlap);
}

void AC_GridLaser::BeginPlay()
{
	Super::BeginPlay();
}

void AC_GridLaser::Initialize(UC_MonsterASC* InInstigatorASC, AActor* InInstigatorActor,
	TSubclassOf<UGameplayEffect> InDamageEffectClass, float InDamageAmount,
	float InBeamLength, float InBeamWidth, float InPreviewTime, float InFireDuration)
{
	instigatorASC = InInstigatorASC;
	instigatorActor = InInstigatorActor;
	damageEffectClass = InDamageEffectClass;
	damageAmount = InDamageAmount;
	previewTime = InPreviewTime;
	fireDuration = InFireDuration;

	// 레이저는 로컬 +X 방향으로 뻗음 — 박스와 프리뷰 중심을 앞으로 오프셋
	const FVector boxHalfExtent(InBeamLength * 0.5f, InBeamWidth * 0.5f, 50.f);
	damageBox->SetBoxExtent(boxHalfExtent, false);
	damageBox->SetRelativeLocation(FVector(InBeamLength * 0.5f, 0.f, 0.f));

	previewPlane->SetRelativeLocation(FVector(InBeamLength * 0.5f, 0.f, 0.f));
	previewPlane->SetRelativeScale3D(FVector(InBeamLength / 100.f, InBeamWidth / 100.f, 1.f));

	if (previewTime <= 0.f)
	{
		OnFireStart();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(fireStartTimer, this, &AC_GridLaser::OnFireStart, previewTime, false);
	}

	GetWorld()->GetTimerManager().SetTimer(fireEndTimer, this, &AC_GridLaser::OnFireEnd, previewTime + fireDuration, false);

	UE_LOG(LogTemp, Log, TEXT("[GridLaser] Initialized at %s"), *GetActorLocation().ToString());
}

void AC_GridLaser::CancelLaser()
{
	if (bCancelled) return;
	bCancelled = true;

	GetWorld()->GetTimerManager().ClearTimer(fireStartTimer);
	GetWorld()->GetTimerManager().ClearTimer(fireEndTimer);
	GetWorld()->GetTimerManager().ClearTimer(damagePulseTimer);

	beamFX->Deactivate();
	damageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	previewPlane->SetVisibility(false);

	Destroy();
}

void AC_GridLaser::OnFireStart()
{
	if (bCancelled) return;
	bFiring = true;

	previewPlane->SetVisibility(false);
	beamFX->Activate(true);
	damageBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 0초 딜레이로 즉시 첫 펄스 — 이미 박스 안에 있는 플레이어도 피해 적용
	GetWorld()->GetTimerManager().SetTimer(damagePulseTimer, this, &AC_GridLaser::ApplyDamageTick, 0.5f, true, 0.0f);
}

void AC_GridLaser::OnFireEnd()
{
	if (bCancelled) return;

	beamFX->Deactivate();
	damageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWorld()->GetTimerManager().ClearTimer(damagePulseTimer);

	Destroy();
}

void AC_GridLaser::ApplyDamageTick()
{
	if (bCancelled || !bFiring) return;

	TArray<AActor*> overlapping;
	damageBox->GetOverlappingActors(overlapping);

	for (AActor* actor : overlapping)
	{
		ApplyDamageTo(actor);
	}
}

void AC_GridLaser::ApplyDamageTo(AActor* target)
{
	if (!target || !instigatorASC.IsValid() || !damageEffectClass) return;

	IAbilitySystemInterface* asi = Cast<IAbilitySystemInterface>(target);
	if (!asi) return;

	UAbilitySystemComponent* targetASC = asi->GetAbilitySystemComponent();
	if (!targetASC) return;

	FGameplayEffectContextHandle ctx = instigatorASC->MakeEffectContext();
	if (instigatorActor.IsValid())
		ctx.AddInstigator(instigatorActor.Get(), instigatorActor.Get());

	FGameplayEffectSpecHandle spec = instigatorASC->MakeOutgoingSpec(damageEffectClass, 1.f, ctx);
	if (!spec.IsValid()) return;

	spec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(FName("Data.Damage")), damageAmount);

	targetASC->ApplyGameplayEffectSpecToSelf(*spec.Data);

	UE_LOG(LogTemp, Log, TEXT("[GridLaser] Player hit. damage=%.1f"), damageAmount);
}

void AC_GridLaser::OnHitBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bFiring || bCancelled) return;
	ApplyDamageTo(OtherActor);
}
