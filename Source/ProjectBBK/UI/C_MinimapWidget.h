// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "C_MinimapWidget.generated.h"

class UTexture2D;
class UC_MinimapLevelData;

/**
 * 우상단 미니맵. 디자이너 트리 없이 NativePaint로 직접 그린다 (WBP 불필요 — AC_BBKGameMode가 생성).
 *
 * - 갈 수 있는 길/공터: 레벨의 내비메시를 시작 시 한 번 텍스처로 구워서 표시
 *   (플레이어 위치에서 이어진 영역만 남김 — 벽 너머 내비메시 섬 제외)
 * - 플레이어(초록)가 항상 중심, 월드 +X가 위쪽 고정
 * - 몬스터(빨강) / 보물상자(하양) / 포탈(파랑, 열리기 전엔 흐리게)
 */
UCLASS()
class PROJECTBBK_API UC_MinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 미니맵 한 변 크기 (Slate 단위) — AC_BBKGameMode가 뷰포트 슬롯 크기로 사용
	UPROPERTY(EditAnywhere, Category = "Minimap")
	float mapSize = 260.f;

	// 화면 우상단 모서리에서 띄울 간격
	UPROPERTY(EditAnywhere, Category = "Minimap")
	FVector2D screenMargin = FVector2D(24.f, 24.f);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// 중심(플레이어)에서 미니맵 가장자리까지의 월드 거리 (cm) — 작을수록 확대
	UPROPERTY(EditAnywhere, Category = "Minimap", meta = (ClampMin = "100.0"))
	float viewRadius = 4500.f;

	// 길 텍스처의 긴 변 해상도. 맵이 클수록 텍셀당 거리가 커진다
	UPROPERTY(EditAnywhere, Category = "Minimap|Bake", meta = (ClampMin = "128", ClampMax = "4096"))
	int32 maskResolution = 1024;

	// 텍셀당 최소 월드 거리 (cm) — 작은 맵에서 텍스처가 불필요하게 커지지 않게
	UPROPERTY(EditAnywhere, Category = "Minimap|Bake", meta = (ClampMin = "5.0"))
	float minTexelSize = 20.f;

	// true면 플레이어 위치에서 이어진 영역만 표시
	UPROPERTY(EditAnywhere, Category = "Minimap|Bake")
	bool bOnlyReachableArea = true;

	// 길 데이터가 없는 레벨에서, 내비메시가 이어져 있어도 실제 충돌(벽·기둥·투명벽)로 막힌 곳은 넘어가지 않게 한다.
	// 내비메시에 영향을 주지 않는 메시가 통로를 막고 있는 맵 대비
	UPROPERTY(EditAnywhere, Category = "Minimap|Bake")
	bool bCollisionCheckReach = true;

	// 이어진 영역 판정 시 이 거리(cm) 이하의 내비메시 틈은 이어진 것으로 본다 (높은 단차·계단에서 끊긴 구간 대비).
	// 너무 크면 얇은 벽 너머까지 이어진 것으로 판정된다
	UPROPERTY(EditAnywhere, Category = "Minimap|Bake", meta = (ClampMin = "0.0"))
	float reachBridgeGap = 150.f;

	UPROPERTY(EditAnywhere, Category = "Minimap|Color")
	FLinearColor backgroundColor = FLinearColor(0.f, 0.f, 0.f, 0.55f);

	UPROPERTY(EditAnywhere, Category = "Minimap|Color")
	FLinearColor borderColor = FLinearColor(1.f, 1.f, 1.f, 0.35f);

	// 길 데이터(UC_MinimapLevelData)가 있는 레벨에서, 공터 몬스터·상자를 감싸는 원에 더하는 여유 (cm).
	// 이 원 안의 내비메시가 공터로 표시된다
	UPROPERTY(EditAnywhere, Category = "Minimap|Bake", meta = (ClampMin = "0.0"))
	float clearingMargin = 600.f;

	// 길 데이터가 없는 레벨(공터 하나짜리 방)에서, 내비메시 모양을 외곽만 남긴 단순한 덩어리로 정리할지.
	// 안쪽 구멍(기둥·소품)을 메우고 가장자리 요철을 편다
	UPROPERTY(EditAnywhere, Category = "Minimap|Bake")
	bool bSimplifyNavShape = true;

	// 위 정리에서 메우는 틈·요철의 크기 (cm) — 클수록 더 네모반듯해진다
	UPROPERTY(EditAnywhere, Category = "Minimap|Bake", meta = (ClampMin = "0.0"))
	float navSimplifyRadius = 300.f;

	// 공터 중심에서 가장 가까운 길까지 이어 주는 통로의 폭 (cm)
	UPROPERTY(EditAnywhere, Category = "Minimap|Bake", meta = (ClampMin = "100.0"))
	float clearingLinkWidth = 600.f;

	// 둘레가 이보다 짧은 외곽선(나무 하나가 만든 구멍 등)은 선으로 그리지 않는다 (cm)
	UPROPERTY(EditAnywhere, Category = "Minimap|Bake", meta = (ClampMin = "0.0"))
	float minContourLength = 2500.f;

	// 길 가장자리 선 굵기 (Slate 단위)
	UPROPERTY(EditAnywhere, Category = "Minimap|Color", meta = (ClampMin = "0.5"))
	float pathEdgeThickness = 2.f;

	// 길 안쪽 채움색 — 선만 보이게 하려면 알파를 0으로
	UPROPERTY(EditAnywhere, Category = "Minimap|Color")
	FLinearColor pathColor = FLinearColor(0.75f, 0.75f, 0.7f, 0.22f);

	// 길 가장자리 선
	UPROPERTY(EditAnywhere, Category = "Minimap|Color")
	bool bDrawPathEdge = true;

	UPROPERTY(EditAnywhere, Category = "Minimap|Color")
	FLinearColor pathEdgeColor = FLinearColor(0.9f, 0.95f, 1.f, 0.95f);

	UPROPERTY(EditAnywhere, Category = "Minimap|Color")
	FLinearColor playerColor = FLinearColor(0.1f, 1.f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Minimap|Color")
	FLinearColor monsterColor = FLinearColor(1.f, 0.1f, 0.1f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Minimap|Color")
	FLinearColor chestColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Minimap|Color")
	FLinearColor portalColor = FLinearColor(0.15f, 0.45f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Minimap|Dot", meta = (ClampMin = "1.0"))
	float playerDotSize = 12.f;

	UPROPERTY(EditAnywhere, Category = "Minimap|Dot", meta = (ClampMin = "1.0"))
	float monsterDotSize = 8.f;

	UPROPERTY(EditAnywhere, Category = "Minimap|Dot", meta = (ClampMin = "1.0"))
	float chestDotSize = 8.f;

	UPROPERTY(EditAnywhere, Category = "Minimap|Dot", meta = (ClampMin = "1.0"))
	float portalDotSize = 11.f;

	// 표시 대상 목록(몬스터·상자·포탈)을 다시 수집하는 주기 — 소환 몬스터·드랍 대응
	UPROPERTY(EditAnywhere, Category = "Minimap", meta = (ClampMin = "0.05"))
	float actorRefreshInterval = 0.5f;

private:
	// 내비메시 → 길 텍스처. 내비메시·플레이어 Pawn이 준비되지 않았으면 false (다음 주기에 재시도)
	bool TryBakeWalkableMask();

	// 길 마스크의 외곽선을 매끈한 폴리라인(월드 XY)으로 뽑아 contours에 저장.
	// 텍셀 가장자리를 그대로 그리면 사선 길이 계단 모양(짧은 직선 여러 개)으로 보이기 때문
	void BuildContours(const TArray<uint8>& Mask, int32 Width, int32 Height, double MinY, double MaxX, float Texel);

	// 길 가장자리 선 — 각 원소가 이어진 선 하나 (월드 X, Y)
	TArray<TArray<FVector2D>> contours;

	void RefreshTrackedActors();

	// 월드 좌표 → 위젯 로컬 좌표. 미니맵 밖이면 false
	bool WorldToMap(const FVector& WorldLocation, float LocalSize, float EdgePadding, FVector2f& OutLocal) const;

	void PaintDot(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry,
		const FVector2f& LocalCenter, float DotSize, const FLinearColor& Color) const;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> maskTexture;

	// 이 레벨의 길 데이터 — 없으면 nullptr (내비메시 전체를 길로 표시)
	UPROPERTY(Transient)
	TObjectPtr<UC_MinimapLevelData> levelData;

	bool bLevelDataChecked = false;

	// 구운 텍스처의 월드 매핑: U ↔ 월드 Y (증가), V ↔ 월드 X (감소 — 위쪽이 +X)
	FVector2D maskWorldMin = FVector2D::ZeroVector;   // (MinX, MinY)
	FVector2D maskWorldMax = FVector2D::ZeroVector;   // (MaxX, MaxY)
	float maskTexelSize = 0.f;
	int32 maskWidth = 0;
	int32 maskHeight = 0;
	bool bMaskBaked = false;
	float bakeRetryElapsed = 0.f;

	// 이번 프레임 중심(플레이어) 위치·시선 — NativeTick에서 갱신
	FVector centerLocation = FVector::ZeroVector;
	float viewYaw = 0.f;
	bool bHasCenter = false;

	float actorRefreshElapsed = 0.f;
	TArray<TWeakObjectPtr<AActor>> trackedMonsters;
	TArray<TWeakObjectPtr<AActor>> trackedChests;
	TArray<TWeakObjectPtr<AActor>> trackedPortalsActive;
	TArray<TWeakObjectPtr<AActor>> trackedPortalsInactive;

	// NativePaint(const)에서 UV 영역을 갱신하므로 mutable
	mutable FSlateBrush maskBrush;
	FSlateBrush backgroundBrush;
	FSlateBrush dotBrush;
};
