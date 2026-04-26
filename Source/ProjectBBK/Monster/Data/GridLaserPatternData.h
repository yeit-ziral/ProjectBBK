// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GridLaserPatternData.generated.h"

USTRUCT(BlueprintType)
struct FGridLaserPatternRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 lineCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	float gridSpacing = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	float cloneOffset = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
	float chargeDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
	float laserPreviewTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
	float laserFireDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser")
	float laserBeamLength = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser")
	float laserBeamWidth = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float failureDamage = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float cameraBlendTime = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float cameraTopDownArm = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float cameraTopDownPitch = -85.f;
};
