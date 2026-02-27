// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "C_BossBeamPatternGA.generated.h"

class AC_BossBeam;
/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_BossBeamPatternGA : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UC_BossBeamPatternGA();

protected:

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;


protected:

    // 빔 Actor 클래스
    UPROPERTY(EditDefaultsOnly, Category = "Beam")
    TSubclassOf<AC_BossBeam> beamClass;

    // 몇 방향인지 (기본 3)
    UPROPERTY(EditDefaultsOnly, Category = "Beam")
    int32 beamCount = 3;
};
