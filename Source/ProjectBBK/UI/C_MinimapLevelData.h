// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "C_MinimapLevelData.generated.h"

/**
 * 레벨별 미니맵 길 데이터. 랜드스케이프의 길 페인트 레이어를 에디터에서 격자로 구워 둔 것.
 * (레이어 가중치는 런타임에 읽을 수 없어서 미리 구워 둔다)
 *
 * 에셋 위치 규칙: /Game/UI/Minimap/DA_Minimap_<레벨이름> — UC_MinimapWidget이 이름으로 찾아 로드한다.
 * 이 에셋이 없는 레벨은 내비메시 전체를 길로 표시한다.
 *
 * 격자 배치: 행 0이 월드 +X 쪽 끝(미니맵 위쪽), 열 0이 월드 -Y 쪽 끝(미니맵 왼쪽).
 *   행 r, 열 c의 중심 = (worldMin.X + (height - r - 0.5) * texelSize, worldMin.Y + (c + 0.5) * texelSize)
 */
UCLASS(BlueprintType)
class PROJECTBBK_API UC_MinimapLevelData : public UDataAsset
{
	GENERATED_BODY()

public:
	// 격자가 덮는 월드 영역의 최소 모서리 (X, Y)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FVector2D worldMin = FVector2D::ZeroVector;

	// 칸 하나의 월드 크기 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap", meta = (ClampMin = "1.0"))
	float texelSize = 100.f;

	// 열 수 (월드 Y 방향)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	int32 width = 0;

	// 행 수 (월드 X 방향)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	int32 height = 0;

	// width * height 칸, 0 = 길 아님 / 그 외 = 길
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	TArray<uint8> roadMask;

	bool IsValidData() const
	{
		return width > 0 && height > 0 && texelSize > 0.f && roadMask.Num() == width * height;
	}
};
