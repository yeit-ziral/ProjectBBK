// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BossBeamPatternGA.h"
#include "../../Object/C_BossBeam.h"
#include "TimerManager.h"

UC_BossBeamPatternGA::UC_BossBeamPatternGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_BossBeamPatternGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    AActor* boss = GetAvatarActorFromActorInfo();
    if (!boss || !beamClass)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // 각도 계산 (360 / beamCount)
    float angleStep = 360.f / beamCount;

    for (int32 i = 0; i < beamCount; ++i)
    {
        FRotator rot(0.f, angleStep * i, 0.f);

        AC_BossBeam* beam =
            GetWorld()->SpawnActor<AC_BossBeam>(beamClass, boss->GetActorLocation(), rot);

        if (beam)
        {
            beam->ownerBoss = boss;
        }
    }

    // GA는 여기서 바로 종료
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}



