// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "C_SpawnProjectile_AnimNotify.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_SpawnProjectile_AnimNotify : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("FireProjectile");
	}

	UPROPERTY(EditAnywhere, Category = "GAS")
	FGameplayTag FireEventTag;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	
};
