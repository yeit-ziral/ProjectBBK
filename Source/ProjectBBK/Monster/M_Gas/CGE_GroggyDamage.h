// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "CGE_GroggyDamage.generated.h"

/**
 * 몬스터 그로기 누적 GE — Set by Caller (Data.Groggy) → ReceivedGroggy 가산
 * Blueprint에서 이 클래스를 부모로 생성 후 그대로 사용
 */
UCLASS()
class PROJECTBBK_API UCGE_GroggyDamage : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UCGE_GroggyDamage();
};
