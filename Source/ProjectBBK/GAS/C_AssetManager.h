// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "C_AssetManager.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_AssetManager : public UAssetManager
{
	GENERATED_BODY()
	
public:
	virtual void StartInitialLoading() override;
};
