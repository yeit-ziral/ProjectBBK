// Fill out your copyright notice in the Description page of Project Settings.


#include "C_PlayerRangedProjectile.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SphereComponent.h"
#include "ProjectBBK/Monster/C_BaseMonster.h"

AC_PlayerRangedProjectile::AC_PlayerRangedProjectile()
{
	// 기본 수명: 5초 후 자동 소멸
	InitialLifeSpan = 5.0f;
}

void AC_PlayerRangedProjectile::Initialize(UAbilitySystemComponent* InstigatorASC,
                                            AActor* InstigatorActor,
                                            TSubclassOf<UGameplayEffect> DamageEffectClass,
                                            float DamageAmount)
{
	instigatorASC     = InstigatorASC;
	instigatorActor   = InstigatorActor;
	damageEffectClass = DamageEffectClass;
	damageAmount      = DamageAmount;
}

void AC_PlayerRangedProjectile::BeginPlay()
{
	Super::BeginPlay();

	// collision은 부모 클래스(AC_RangedProjectile)에서 생성됨
	if (collision)
	{
		collision->OnComponentBeginOverlap.AddDynamic(
			this, &AC_PlayerRangedProjectile::OnProjectileOverlap);
	}
}

void AC_PlayerRangedProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent,
                                                     AActor* OtherActor,
                                                     UPrimitiveComponent* OtherComp,
                                                     int32 OtherBodyIndex,
                                                     bool bFromSweep,
                                                     const FHitResult& SweepResult)
{
	// 자기 자신이나 발사자 무시
	if (!OtherActor || OtherActor == this || OtherActor == instigatorActor.Get())
	{
		return;
	}

	// 이미 히트 처리됨
	if (bAlreadyHit)
	{
		return;
	}

	// 몬스터에게만 데미지 적용
	AC_BaseMonster* monster = Cast<AC_BaseMonster>(OtherActor);
	if (!monster)
	{
		return;
	}

	if (instigatorASC.IsValid() && damageEffectClass)
	{
		UAbilitySystemComponent* targetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);

		if (targetASC)
		{
			FGameplayEffectContextHandle effectContext = instigatorASC->MakeEffectContext();
			effectContext.AddSourceObject(instigatorActor.Get());

			FGameplayEffectSpecHandle specHandle =
				instigatorASC->MakeOutgoingSpec(damageEffectClass, 1.0f, effectContext);

			if (specHandle.IsValid())
			{
				// Set by Caller: Data.Damage 태그로 데미지 값 주입
				UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
					specHandle,
					FGameplayTag::RequestGameplayTag(FName("Data.Damage")),
					damageAmount);

				instigatorASC->ApplyGameplayEffectSpecToTarget(
					*specHandle.Data.Get(), targetASC);
			}
		}
	}

	bAlreadyHit = true;
	Destroy();
}
