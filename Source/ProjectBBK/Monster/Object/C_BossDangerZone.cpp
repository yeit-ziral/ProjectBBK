// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BossDangerZone.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "../../GAS/Attributes/C_ChracterAttributeSetBase.h"

AC_BossDangerZone::AC_BossDangerZone()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AC_BossDangerZone::InitDangerZone(
	UAbilitySystemComponent* InBossASC,
	TSubclassOf<UGameplayEffect> InDamageGEClass,
	FVector InSafeZoneCenter,
	float InSafeZoneRadius,
	float InHealthPercent,
	float InTickRate)
{
	bossASC        = InBossASC;
	damageGEClass  = InDamageGEClass;
	safeZoneCenter = InSafeZoneCenter;
	safeZoneRadius = InSafeZoneRadius;
	healthPercent  = InHealthPercent;

	GetWorld()->GetTimerManager().SetTimer(
		damageTimerHandle, this, &AC_BossDangerZone::TickDamage, InTickRate, true);
}

void AC_BossDangerZone::StopDangerZone()
{
	if (UWorld* world = GetWorld())
		world->GetTimerManager().ClearTimer(damageTimerHandle);
}

void AC_BossDangerZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopDangerZone();
	Super::EndPlay(EndPlayReason);
}

void AC_BossDangerZone::TickDamage()
{
	if (!bossASC.IsValid() || !damageGEClass) return;

	APawn* player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!player) return;

	// 안전지대 판정 — 높이(Z) 무시, XY 평면 거리만 사용
	FVector delta = player->GetActorLocation() - safeZoneCenter;
	delta.Z = 0.f;
	if (delta.Size() <= safeZoneRadius) return;

	IAbilitySystemInterface* ascInterface = Cast<IAbilitySystemInterface>(player);
	if (!ascInterface) return;

	UAbilitySystemComponent* targetASC = ascInterface->GetAbilitySystemComponent();
	if (!targetASC) return;

	// 플레이어 최대 체력 조회 후 비율 계산
	const float maxHealth = targetASC->GetNumericAttribute(
		UC_ChracterAttributeSetBase::GetmaxHealthAttribute());
	if (maxHealth <= 0.f) return;

	const float damage = maxHealth * healthPercent;

	FGameplayEffectContextHandle context = bossASC->MakeEffectContext();
	context.AddInstigator(GetOwner(), GetOwner());

	FGameplayEffectSpecHandle spec = bossASC->MakeOutgoingSpec(damageGEClass, 1.f, context);
	if (!spec.IsValid()) return;

	spec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag("Data.Damage"), damage);

	bossASC->ApplyGameplayEffectSpecToTarget(*spec.Data.Get(), targetASC);
}
