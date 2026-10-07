// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MinimapWidget.h"
#include "C_MinimapLevelData.h"
#include "../Monster/C_BaseMonster.h"
#include "../Monster/Object/C_MonsterArenaLock.h"
#include "../Items/C_BaseItem.h"
#include "Misc/PackageName.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/BlockingVolume.h"
#include "Components/BrushComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "../Items/C_TreasureChest.h"
#include "../LevelSystem/C_Portal.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateRoundedBoxBrush.h"

// 길 텍스처를 테두리 안쪽으로 들여 그리는 폭 (Slate 단위)
static const float MinimapInnerPadding = 3.f;

// 내비메시가 아직 없을 때 굽기를 다시 시도하는 주기 (초)
static const float MinimapBakeRetryInterval = 0.5f;

void UC_MinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 마우스 입력을 가로채지 않게
	SetVisibility(ESlateVisibility::HitTestInvisible);

	backgroundBrush = FSlateRoundedBoxBrush(backgroundColor, 8.f, borderColor, 1.5f);
	dotBrush = FSlateRoundedBoxBrush(FLinearColor::White);   // 반지름 = 높이의 절반 → 정사각형으로 그리면 원

	bMaskBaked = false;
	bakeRetryElapsed = MinimapBakeRetryInterval;   // 첫 틱에 바로 시도
	actorRefreshElapsed = actorRefreshInterval;
}

void UC_MinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 캐릭터 교체로 Pawn이 바뀌므로 매 프레임 다시 얻는다
	const APawn* Pawn = GetOwningPlayerPawn();
	bHasCenter = Pawn != nullptr;
	if (Pawn)
	{
		centerLocation = Pawn->GetActorLocation();
		const APlayerController* PC = GetOwningPlayer();
		viewYaw = PC ? PC->GetControlRotation().Yaw : Pawn->GetActorRotation().Yaw;
	}

	if (!bMaskBaked)
	{
		bakeRetryElapsed += InDeltaTime;
		if (bakeRetryElapsed >= MinimapBakeRetryInterval)
		{
			bakeRetryElapsed = 0.f;
			bMaskBaked = TryBakeWalkableMask();
		}
	}

	actorRefreshElapsed += InDeltaTime;
	if (actorRefreshElapsed >= actorRefreshInterval)
	{
		actorRefreshElapsed = 0.f;
		RefreshTrackedActors();
	}
}

void UC_MinimapWidget::RefreshTrackedActors()
{
	trackedMonsters.Reset();
	trackedChests.Reset();
	trackedPortalsActive.Reset();
	trackedPortalsInactive.Reset();

	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<AC_BaseMonster> It(World); It; ++It)
	{
		// 사망 처리된 몬스터는 소멸 전까지 캡슐 콜리전이 꺼져 있다 — 죽는 순간 점이 사라지게
		const UCapsuleComponent* Capsule = It->GetCapsuleComponent();
		if (Capsule && Capsule->GetCollisionEnabled() == ECollisionEnabled::NoCollision) continue;

		trackedMonsters.Add(*It);
	}

	for (TActorIterator<AC_TreasureChest> It(World); It; ++It)
	{
		trackedChests.Add(*It);
	}

	for (TActorIterator<AC_Portal> It(World); It; ++It)
	{
		if (It->IsActivated())
			trackedPortalsActive.Add(*It);
		else
			trackedPortalsInactive.Add(*It);
	}
}

bool UC_MinimapWidget::TryBakeWalkableMask()
{
	UWorld* World = GetWorld();
	const APawn* Pawn = GetOwningPlayerPawn();
	if (!World || !Pawn) return false;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	const ARecastNavMesh* NavMesh = NavSys ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance()) : nullptr;

	// 레벨별 길 데이터(/Game/UI/Minimap/DA_Minimap_<레벨이름>)가 있으면 그것이 길, 없으면 내비메시 전체가 길
	if (!bLevelDataChecked)
	{
		bLevelDataChecked = true;
		const FString LevelName = FPackageName::GetShortName(UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()));
		const FString AssetPath = FString::Printf(TEXT("/Game/UI/Minimap/DA_Minimap_%s.DA_Minimap_%s"), *LevelName, *LevelName);
		levelData = LoadObject<UC_MinimapLevelData>(nullptr, *AssetPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	const bool bUseRoadData = levelData && levelData->IsValidData();

	// 타일 인덱스(int32) 버전 GetPolysInTile은 쓰면 안 된다 — 내부에서 salt 0으로 타일 참조를 만들어서
	// 로드 시 재생성된 내비메시(salt ≥ 1)에서는 모든 타일이 빈 것으로 나온다
	TArray<FNavTileRef> TileRefs;
	if (NavMesh)
	{
		NavMesh->GetAllNavMeshTiles(TileRefs);
	}

	float Texel = 0.f;
	int32 Width = 0;
	int32 Height = 0;
	double MinX = 0.0;
	double MinY = 0.0;
	double MaxX = 0.0;
	double MaxY = 0.0;

	// 가장자리 1텍셀은 항상 비워 둔다 — 텍스처 밖(Clamp 샘플링)이 투명으로 나오게
	if (bUseRoadData)
	{
		Texel = levelData->texelSize;
		Width = levelData->width + 2;
		Height = levelData->height + 2;
		MinX = levelData->worldMin.X;
		MinY = levelData->worldMin.Y;
		MaxX = MinX + static_cast<double>(levelData->height) * Texel;
		MaxY = MinY + static_cast<double>(levelData->width) * Texel;
	}
	else
	{
		if (!NavMesh) return false;

		const FBox Bounds = NavMesh->GetNavMeshBounds();
		if (TileRefs.Num() <= 0 || !Bounds.IsValid) return false;

		const FVector BoundsSize = Bounds.GetSize();
		const float LongSide = static_cast<float>(FMath::Max(BoundsSize.X, BoundsSize.Y));
		Texel = FMath::Max(LongSide / FMath::Max(maskResolution, 1), minTexelSize);
		Width = FMath::CeilToInt32(BoundsSize.Y / Texel) + 2;
		Height = FMath::CeilToInt32(BoundsSize.X / Texel) + 2;
		MinX = Bounds.Min.X;
		MinY = Bounds.Min.Y;
		MaxX = Bounds.Max.X;
		MaxY = Bounds.Max.Y;
	}
	if (Width <= 2 || Height <= 2 || Width > 8192 || Height > 8192) return false;

	TArray<uint8> Mask;
	Mask.SetNumZeroed(Width * Height);

	// 칸별 바닥 높이 — 충돌로 막힌 곳을 가려낼 때(길 데이터가 없는 레벨) 스윕 높이로 쓴다
	const float NoHeight = TNumericLimits<float>::Lowest();
	const bool bNeedHeights = !bUseRoadData && bOnlyReachableArea && bCollisionCheckReach;
	TArray<float> Heights;
	if (bNeedHeights)
	{
		Heights.Init(NoHeight, Width * Height);
	}

	// 현재 래스터화 중인 폴리곤의 평면 (중심 + 법선) — 경사·계단에서도 칸 위치의 높이를 정확히 구하려고
	FVector PolyCenter = FVector::ZeroVector;
	FVector PolyNormal = FVector::UpVector;

	auto MarkPixel = [&Mask, &Heights, &PolyCenter, &PolyNormal, bNeedHeights, Width, Height, MinY, MaxX, Texel](int32 Px, int32 Py)
	{
		if (Px < 1 || Px >= Width - 1 || Py < 1 || Py >= Height - 1) return;

		Mask[Py * Width + Px] = 1;
		if (bNeedHeights)
		{
			const double WorldX = MaxX - (Py - 0.5) * Texel;
			const double WorldY = MinY + (Px - 0.5) * Texel;
			const double Z = FMath::Abs(PolyNormal.Z) > 0.05
				? PolyCenter.Z - (PolyNormal.X * (WorldX - PolyCenter.X) + PolyNormal.Y * (WorldY - PolyCenter.Y)) / PolyNormal.Z
				: PolyCenter.Z;
			Heights[Py * Width + Px] = static_cast<float>(Z);
		}
	};

	// ── 내비메시 폴리곤(볼록)을 스캔라인으로 래스터화 ──
	TArray<FNavPoly> Polys;
	TArray<FVector> Verts;
	TArray<FVector2D> Pixels;
	int32 PolyCount = 0;

	for (const FNavTileRef& TileRef : TileRefs)
	{
		Polys.Reset();
		if (!NavMesh->GetPolysInTile(TileRef, Polys)) continue;

		for (const FNavPoly& Poly : Polys)
		{
			Verts.Reset();
			if (!NavMesh->GetPolyVerts(Poly.Ref, Verts) || Verts.Num() < 3) continue;
			++PolyCount;

			// 폴리곤 평면 (Newell 방식 법선)
			PolyCenter = FVector::ZeroVector;
			PolyNormal = FVector::ZeroVector;
			for (int32 i = 0; i < Verts.Num(); ++i)
			{
				const FVector& A = Verts[i];
				const FVector& B = Verts[(i + 1) % Verts.Num()];
				PolyCenter += A;
				PolyNormal.X += (A.Y - B.Y) * (A.Z + B.Z);
				PolyNormal.Y += (A.Z - B.Z) * (A.X + B.X);
				PolyNormal.Z += (A.X - B.X) * (A.Y + B.Y);
			}
			PolyCenter /= Verts.Num();
			if (!PolyNormal.Normalize()) PolyNormal = FVector::UpVector;

			Pixels.Reset();
			double RowMin = TNumericLimits<double>::Max();
			double RowMax = TNumericLimits<double>::Lowest();
			for (const FVector& V : Verts)
			{
				const FVector2D P((V.Y - MinY) / Texel + 1.0, (MaxX - V.X) / Texel + 1.0);
				Pixels.Add(P);
				RowMin = FMath::Min(RowMin, P.Y);
				RowMax = FMath::Max(RowMax, P.Y);

				// 텍셀보다 얇은 폴리곤이 스캔라인 사이로 빠지지 않게 꼭짓점도 찍는다
				MarkPixel(FMath::FloorToInt32(P.X), FMath::FloorToInt32(P.Y));
			}

			const int32 RowStart = FMath::Max(FMath::CeilToInt32(RowMin - 0.5), 1);
			const int32 RowEnd = FMath::Min(FMath::FloorToInt32(RowMax - 0.5), Height - 2);
			for (int32 Row = RowStart; Row <= RowEnd; ++Row)
			{
				const double ScanY = Row + 0.5;
				double XMin = TNumericLimits<double>::Max();
				double XMax = TNumericLimits<double>::Lowest();

				for (int32 i = 0; i < Pixels.Num(); ++i)
				{
					const FVector2D& A = Pixels[i];
					const FVector2D& B = Pixels[(i + 1) % Pixels.Num()];
					if ((A.Y <= ScanY && B.Y > ScanY) || (B.Y <= ScanY && A.Y > ScanY))
					{
						const double X = A.X + (ScanY - A.Y) / (B.Y - A.Y) * (B.X - A.X);
						XMin = FMath::Min(XMin, X);
						XMax = FMath::Max(XMax, X);
					}
				}

				if (XMin > XMax) continue;

				const int32 ColStart = FMath::Max(FMath::CeilToInt32(XMin - 0.5), 1);
				const int32 ColEnd = FMath::Min(FMath::FloorToInt32(XMax - 0.5), Width - 2);
				for (int32 Col = ColStart; Col <= ColEnd; ++Col)
				{
					MarkPixel(Col, Row);
				}
			}
		}
	}

	if (bUseRoadData)
	{
		// ── 길 = 구워 둔 길 레이어 + 공터(공터 범위 안의 내비메시) ──
		// 공터는 길 레이어로 칠해져 있지 않아서, 공터 잠금 액터에 묶인 몬스터·상자를 감싸는 원으로 범위를 잡는다
		TArray<uint8> InClearing;
		InClearing.SetNumZeroed(Width * Height);

		for (TActorIterator<AC_MonsterArenaLock> It(World); It; ++It)
		{
			TArray<FVector> Points;
			for (const AC_BaseMonster* Monster : It->GetGuardMonsters())
			{
				if (Monster) Points.Add(Monster->GetActorLocation());
			}
			for (const AC_BaseItem* Chest : It->GetLockedChests())
			{
				if (Chest) Points.Add(Chest->GetActorLocation());
			}
			if (Points.Num() == 0) continue;

			FVector Center = FVector::ZeroVector;
			for (const FVector& P : Points) Center += P;
			Center /= Points.Num();

			double Radius = 0.0;
			for (const FVector& P : Points) Radius = FMath::Max(Radius, FVector::Dist2D(P, Center));
			Radius += clearingMargin;

			const double CenterCol = (Center.Y - MinY) / Texel + 1.0;
			const double CenterRow = (MaxX - Center.X) / Texel + 1.0;
			const double RadiusTexels = Radius / Texel;
			const int32 RowFrom = FMath::Max(FMath::FloorToInt32(CenterRow - RadiusTexels), 1);
			const int32 RowTo = FMath::Min(FMath::CeilToInt32(CenterRow + RadiusTexels), Height - 2);
			const int32 ColFrom = FMath::Max(FMath::FloorToInt32(CenterCol - RadiusTexels), 1);
			const int32 ColTo = FMath::Min(FMath::CeilToInt32(CenterCol + RadiusTexels), Width - 2);
			for (int32 Row = RowFrom; Row <= RowTo; ++Row)
			{
				for (int32 Col = ColFrom; Col <= ColTo; ++Col)
				{
					const double DRow = Row + 0.5 - CenterRow;
					const double DCol = Col + 0.5 - CenterCol;
					if (DRow * DRow + DCol * DCol <= RadiusTexels * RadiusTexels && !InClearing[Row * Width + Col])
					{
						InClearing[Row * Width + Col] = 1;
					}
				}
			}

			// ── 공터 ↔ 길 연결 통로: 공터 중심에서 가장 가까운 길 칸까지 띠를 긋는다 ──
			double BestDistSq = TNumericLimits<double>::Max();
			double BestCol = 0.0;
			double BestRow = 0.0;
			for (int32 Row = 0; Row < levelData->height; ++Row)
			{
				for (int32 Col = 0; Col < levelData->width; ++Col)
				{
					if (!levelData->roadMask[Row * levelData->width + Col]) continue;
					const double DRow = Row + 1.5 - CenterRow;
					const double DCol = Col + 1.5 - CenterCol;
					const double DistSq = DRow * DRow + DCol * DCol;
					if (DistSq < BestDistSq)
					{
						BestDistSq = DistSq;
						BestCol = Col + 1.5;
						BestRow = Row + 1.5;
					}
				}
			}

			if (BestDistSq < TNumericLimits<double>::Max())
			{
				const double HalfWidth = FMath::Max(clearingLinkWidth * 0.5 / Texel, 1.0);
				const double SegCol = BestCol - CenterCol;
				const double SegRow = BestRow - CenterRow;
				const double SegLenSq = FMath::Max(SegCol * SegCol + SegRow * SegRow, 1e-6);

				const int32 LinkRowFrom = FMath::Max(FMath::FloorToInt32(FMath::Min(CenterRow, BestRow) - HalfWidth), 1);
				const int32 LinkRowTo = FMath::Min(FMath::CeilToInt32(FMath::Max(CenterRow, BestRow) + HalfWidth), Height - 2);
				const int32 LinkColFrom = FMath::Max(FMath::FloorToInt32(FMath::Min(CenterCol, BestCol) - HalfWidth), 1);
				const int32 LinkColTo = FMath::Min(FMath::CeilToInt32(FMath::Max(CenterCol, BestCol) + HalfWidth), Width - 2);
				for (int32 Row = LinkRowFrom; Row <= LinkRowTo; ++Row)
				{
					for (int32 Col = LinkColFrom; Col <= LinkColTo; ++Col)
					{
						// 선분(공터 중심 → 길)까지의 거리
						const double PCol = Col + 0.5 - CenterCol;
						const double PRow = Row + 0.5 - CenterRow;
						const double Along = FMath::Clamp((PCol * SegCol + PRow * SegRow) / SegLenSq, 0.0, 1.0);
						const double DCol = PCol - SegCol * Along;
						const double DRow = PRow - SegRow * Along;
						if (DCol * DCol + DRow * DRow <= HalfWidth * HalfWidth)
						{
							InClearing[Row * Width + Col] = 2;   // 2 = 통로 (내비메시와 무관하게 표시)
						}
					}
				}
			}
		}

		const int32 DataWidth = levelData->width;
		const int32 DataHeight = levelData->height;
		for (int32 Row = 0; Row < DataHeight; ++Row)
		{
			for (int32 Col = 0; Col < DataWidth; ++Col)
			{
				const int32 Index = (Row + 1) * Width + (Col + 1);
				const bool bRoad = levelData->roadMask[Row * DataWidth + Col] != 0;
				Mask[Index] = (bRoad || InClearing[Index] == 2 || (Mask[Index] && InClearing[Index])) ? 1 : 0;
			}
		}

		// ── 투명벽(BlockingVolume) 너머로 이어진 길은 지운다 ──
		// 벽이 지나는 칸을 막힌 칸으로 두고, 플레이어 위치에서 번져 나가 닿는 칸만 남긴다
		if (bOnlyReachableArea)
		{
			TArray<uint8> Blocked;
			Blocked.SetNumZeroed(Width * Height);

			// 벽이 텍셀보다 얇아도 칸 중심 검사에서 빠지지 않게 두께를 보탠다
			const double WallPad = Texel * 0.75;

			for (TActorIterator<ABlockingVolume> It(World); It; ++It)
			{
				const UBrushComponent* BrushComp = It->GetBrushComponent();
				if (!BrushComp || !BrushComp->BrushBodySetup) continue;
				if (BrushComp->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block) continue;

				const FBox LocalBox = BrushComp->BrushBodySetup->AggGeom.CalcAABB(FTransform::Identity);
				if (!LocalBox.IsValid) continue;

				const FTransform& ToWorld = BrushComp->GetComponentTransform();
				const FVector AbsScale = ToWorld.GetScale3D().GetAbs();
				const double PadX = WallPad / FMath::Max(AbsScale.X, 0.01);
				const double PadY = WallPad / FMath::Max(AbsScale.Y, 0.01);

				const FBox WorldBox = LocalBox.TransformBy(ToWorld).ExpandBy(WallPad);
				const double CenterZ = WorldBox.GetCenter().Z;

				const int32 ColFrom = FMath::Max(FMath::FloorToInt32((WorldBox.Min.Y - MinY) / Texel + 1.0), 1);
				const int32 ColTo = FMath::Min(FMath::CeilToInt32((WorldBox.Max.Y - MinY) / Texel + 1.0), Width - 2);
				const int32 RowFrom = FMath::Max(FMath::FloorToInt32((MaxX - WorldBox.Max.X) / Texel + 1.0), 1);
				const int32 RowTo = FMath::Min(FMath::CeilToInt32((MaxX - WorldBox.Min.X) / Texel + 1.0), Height - 2);
				for (int32 Row = RowFrom; Row <= RowTo; ++Row)
				{
					for (int32 Col = ColFrom; Col <= ColTo; ++Col)
					{
						// 칸 중심의 월드 좌표 → 벽의 로컬 좌표. 높이는 따지지 않는다 (위에서 내려다본 평면 판정)
						const FVector WorldPoint(MaxX - (Row - 0.5) * Texel, MinY + (Col - 0.5) * Texel, CenterZ);
						const FVector LocalPoint = ToWorld.InverseTransformPosition(WorldPoint);
						if (LocalPoint.X >= LocalBox.Min.X - PadX && LocalPoint.X <= LocalBox.Max.X + PadX
							&& LocalPoint.Y >= LocalBox.Min.Y - PadY && LocalPoint.Y <= LocalBox.Max.Y + PadY)
						{
							Blocked[Row * Width + Col] = 1;
						}
					}
				}
			}

			// 시작점 = 플레이어 아래 칸. 길 위가 아니면 가까운 길 칸을 찾는다
			const FVector PawnLocation = Pawn->GetActorLocation();
			const int32 SeedCol = FMath::Clamp(FMath::FloorToInt32((PawnLocation.Y - MinY) / Texel + 1.0), 0, Width - 1);
			const int32 SeedRow = FMath::Clamp(FMath::FloorToInt32((MaxX - PawnLocation.X) / Texel + 1.0), 0, Height - 1);

			int32 SeedIndex = INDEX_NONE;
			const int32 SeedSearchRadius = 30;
			for (int32 Radius = 0; Radius <= SeedSearchRadius && SeedIndex == INDEX_NONE; ++Radius)
			{
				for (int32 Row = SeedRow - Radius; Row <= SeedRow + Radius && SeedIndex == INDEX_NONE; ++Row)
				{
					for (int32 Col = SeedCol - Radius; Col <= SeedCol + Radius; ++Col)
					{
						if (Row < 0 || Row >= Height || Col < 0 || Col >= Width) continue;
						if (Mask[Row * Width + Col] && !Blocked[Row * Width + Col])
						{
							SeedIndex = Row * Width + Col;
							break;
						}
					}
				}
			}

			// 시작점을 못 찾으면(플레이어가 길에서 멀리 있음) 걸러내지 않고 전체를 표시
			if (SeedIndex != INDEX_NONE)
			{
				TArray<uint8> Reached;
				Reached.SetNumZeroed(Width * Height);

				TArray<int32> Stack;
				Stack.Reserve(Width * 4);
				Stack.Push(SeedIndex);
				Reached[SeedIndex] = 1;

				auto Visit = [&Mask, &Blocked, &Reached, &Stack](int32 Index)
				{
					if (Mask[Index] && !Blocked[Index] && !Reached[Index])
					{
						Reached[Index] = 1;
						Stack.Push(Index);
					}
				};

				while (Stack.Num() > 0)
				{
					const int32 Index = Stack.Pop(EAllowShrinking::No);
					const int32 Col = Index % Width;
					const int32 Row = Index / Width;
					if (Col > 0) Visit(Index - 1);
					if (Col < Width - 1) Visit(Index + 1);
					if (Row > 0) Visit(Index - Width);
					if (Row < Height - 1) Visit(Index + Width);
				}

				for (int32 i = 0; i < Mask.Num(); ++i)
				{
					Mask[i] = Mask[i] && Reached[i];
				}
			}
		}
	}
	// 타일은 있는데 폴리곤이 없다 = 내비메시가 아직 빌드 중 → 다음 주기에 재시도
	else if (PolyCount == 0)
	{
		return false;
	}
	// ── 플레이어 위치에서 이어진 영역만 남김 ──
	else if (bOnlyReachableArea)
	{
		// 연결 판정용으로만 팽창시킨 사본 — 좁은 틈(단차에서 끊긴 내비메시)을 이어 준다. 표시되는 모양은 원본 그대로
		TArray<uint8> Bridged = Mask;
		const int32 BridgeRadius = FMath::CeilToInt32(reachBridgeGap / (2.f * Texel));
		if (BridgeRadius > 0)
		{
			TArray<uint8> Temp;
			Temp.SetNumZeroed(Width * Height);

			for (int32 Row = 0; Row < Height; ++Row)
			{
				for (int32 Col = 0; Col < Width; ++Col)
				{
					if (!Mask[Row * Width + Col]) continue;
					const int32 From = FMath::Max(Col - BridgeRadius, 0);
					const int32 To = FMath::Min(Col + BridgeRadius, Width - 1);
					for (int32 c = From; c <= To; ++c) Temp[Row * Width + c] = 1;
				}
			}

			for (int32 Row = 0; Row < Height; ++Row)
			{
				for (int32 Col = 0; Col < Width; ++Col)
				{
					if (!Temp[Row * Width + Col]) continue;
					const int32 From = FMath::Max(Row - BridgeRadius, 0);
					const int32 To = FMath::Min(Row + BridgeRadius, Height - 1);
					for (int32 r = From; r <= To; ++r) Bridged[r * Width + Col] = 1;
				}
			}
		}

		// 시작점 = 플레이어 아래 텍셀. 비어 있으면 가까운 칸을 찾는다
		const FVector PawnLocation = Pawn->GetActorLocation();
		const int32 SeedCol = FMath::Clamp(FMath::FloorToInt32((PawnLocation.Y - MinY) / Texel + 1.0), 0, Width - 1);
		const int32 SeedRow = FMath::Clamp(FMath::FloorToInt32((MaxX - PawnLocation.X) / Texel + 1.0), 0, Height - 1);

		int32 SeedIndex = INDEX_NONE;
		const int32 SeedSearchRadius = 30;
		for (int32 Radius = 0; Radius <= SeedSearchRadius && SeedIndex == INDEX_NONE; ++Radius)
		{
			for (int32 Row = SeedRow - Radius; Row <= SeedRow + Radius && SeedIndex == INDEX_NONE; ++Row)
			{
				for (int32 Col = SeedCol - Radius; Col <= SeedCol + Radius; ++Col)
				{
					if (Row < 0 || Row >= Height || Col < 0 || Col >= Width) continue;
					if (Bridged[Row * Width + Col])
					{
						SeedIndex = Row * Width + Col;
						break;
					}
				}
			}
		}

		// 시작점을 못 찾으면(플레이어가 내비메시 밖) 걸러내지 않고 전체를 표시
		if (SeedIndex != INDEX_NONE)
		{
			TArray<uint8> Reached;
			Reached.SetNumZeroed(Width * Height);

			TArray<int32> Stack;
			Stack.Reserve(Width * 4);
			Stack.Push(SeedIndex);
			Reached[SeedIndex] = 1;

			// 내비메시는 벽·기둥·투명벽을 뚫고 이어져 있을 수 있다(내비에 영향을 주지 않는 메시 등).
			// 칸에서 칸으로 번질 때마다 그 사이를 실제 충돌(Pawn 채널)로 쓸어 보고, 막혀 있으면 넘어가지 않는다
			TArray<float> ReachedZ;
			FCollisionQueryParams SweepParams(SCENE_QUERY_STAT(MinimapReach), false);
			const FCollisionShape SweepShape = FCollisionShape::MakeSphere(25.f);
			const float SweepHeight = 70.f;   // 바닥·낮은 턱에 걸리지 않게 띄우는 높이
			if (bNeedHeights)
			{
				ReachedZ.Init(0.f, Width * Height);
				ReachedZ[SeedIndex] = Heights[SeedIndex] != NoHeight
					? Heights[SeedIndex]
					: static_cast<float>(PawnLocation.Z - Pawn->GetSimpleCollisionHalfHeight());

				// 몬스터·플레이어는 벽이 아니다
				for (TActorIterator<APawn> It(World); It; ++It)
				{
					SweepParams.AddIgnoredActor(*It);
				}
			}

			auto Visit = [&Bridged, &Reached, &Stack, &Heights, &ReachedZ, &SweepParams, &SweepShape,
				World, bNeedHeights, NoHeight, SweepHeight, Width, MinY, MaxX, Texel](int32 Index, int32 From)
			{
				if (!Bridged[Index] || Reached[Index]) return;

				if (bNeedHeights)
				{
					// 내비메시가 없는 칸(틈을 메운 칸)은 온 쪽의 높이를 이어받는다
					const float ToZ = Heights[Index] != NoHeight ? Heights[Index] : ReachedZ[From];
					const FVector Start(MaxX - (From / Width - 0.5) * Texel, MinY + (From % Width - 0.5) * Texel, ReachedZ[From] + SweepHeight);
					const FVector End(MaxX - (Index / Width - 0.5) * Texel, MinY + (Index % Width - 0.5) * Texel, ToZ + SweepHeight);

					// 막혔어도 Reached로 표시하지 않는다 — 다른 방향에서는 닿을 수 있다
					if (World->SweepTestByChannel(Start, End, FQuat::Identity, ECC_Pawn, SweepShape, SweepParams)) return;

					ReachedZ[Index] = ToZ;
				}

				Reached[Index] = 1;
				Stack.Push(Index);
			};

			while (Stack.Num() > 0)
			{
				const int32 Index = Stack.Pop(EAllowShrinking::No);
				const int32 Col = Index % Width;
				const int32 Row = Index / Width;
				if (Col > 0) Visit(Index - 1, Index);
				if (Col < Width - 1) Visit(Index + 1, Index);
				if (Row > 0) Visit(Index - Width, Index);
				if (Row < Height - 1) Visit(Index + Width, Index);
			}

			for (int32 i = 0; i < Mask.Num(); ++i)
			{
				Mask[i] = Mask[i] && Reached[i];
			}
		}
	}

	// ── 길 데이터가 없는 레벨(공터 하나짜리 방): 외곽만 남긴 단순한 모양으로 ──
	// 내비메시 그대로면 기둥·소품이 낸 구멍과 들쭉날쭉한 가장자리까지 다 그려진다
	if (!bUseRoadData && bSimplifyNavShape)
	{
		const int32 SimplifyRadius = FMath::Clamp(FMath::CeilToInt32(navSimplifyRadius / Texel), 0, 64);

		// 가로 → 세로 순서의 사각 팽창/침식. 이미지 밖은 빈 칸으로 취급
		auto Morph = [Width, Height](TArray<uint8>& Data, int32 Radius, bool bDilate)
		{
			if (Radius <= 0) return;

			TArray<uint8> Temp;
			Temp.SetNumUninitialized(Data.Num());
			for (int32 Axis = 0; Axis < 2; ++Axis)
			{
				const TArray<uint8>& Source = Axis == 0 ? Data : Temp;
				TArray<uint8>& Target = Axis == 0 ? Temp : Data;
				const int32 Stride = Axis == 0 ? 1 : Width;
				const int32 Limit = Axis == 0 ? Width : Height;

				for (int32 Row = 0; Row < Height; ++Row)
				{
					for (int32 Col = 0; Col < Width; ++Col)
					{
						const int32 Index = Row * Width + Col;
						const int32 Along = Axis == 0 ? Col : Row;

						// 팽창: 범위 안에 하나라도 차 있으면 1 / 침식: 범위 안이 전부 차 있어야 1
						bool bResult = !bDilate;
						for (int32 Offset = -Radius; Offset <= Radius; ++Offset)
						{
							const int32 Pos = Along + Offset;
							const bool bFilled = Pos >= 0 && Pos < Limit && Source[Index + Offset * Stride] != 0;
							if (bDilate && bFilled) { bResult = true; break; }
							if (!bDilate && !bFilled) { bResult = false; break; }
						}
						Target[Index] = bResult ? 1 : 0;
					}
				}
			}
		};

		// 1) 팽창 — 좁은 틈과 가장자리 요철을 메운다
		Morph(Mask, SimplifyRadius, true);

		// 2) 안쪽 구멍 메우기 — 테두리에서 빈 칸을 따라 번져 닿지 못한 빈 칸은 전부 안쪽
		{
			TArray<uint8> Outside;
			Outside.SetNumZeroed(Width * Height);
			TArray<int32> Stack;
			Stack.Reserve(Width * 4);

			auto Visit = [&Mask, &Outside, &Stack](int32 Index)
			{
				if (!Mask[Index] && !Outside[Index])
				{
					Outside[Index] = 1;
					Stack.Push(Index);
				}
			};

			for (int32 Col = 0; Col < Width; ++Col)
			{
				Visit(Col);
				Visit((Height - 1) * Width + Col);
			}
			for (int32 Row = 0; Row < Height; ++Row)
			{
				Visit(Row * Width);
				Visit(Row * Width + Width - 1);
			}

			while (Stack.Num() > 0)
			{
				const int32 Index = Stack.Pop(EAllowShrinking::No);
				const int32 Col = Index % Width;
				const int32 Row = Index / Width;
				if (Col > 0) Visit(Index - 1);
				if (Col < Width - 1) Visit(Index + 1);
				if (Row > 0) Visit(Index - Width);
				if (Row < Height - 1) Visit(Index + Width);
			}

			for (int32 i = 0; i < Mask.Num(); ++i)
			{
				Mask[i] = Outside[i] ? 0 : 1;
			}
		}

		// 3) 침식 — 팽창으로 불어난 만큼 되돌린다 (메워진 틈·구멍은 그대로 남음)
		Morph(Mask, SimplifyRadius, false);

		// 가장자리 1텍셀은 항상 비워 둔다
		for (int32 Col = 0; Col < Width; ++Col)
		{
			Mask[Col] = 0;
			Mask[(Height - 1) * Width + Col] = 0;
		}
		for (int32 Row = 0; Row < Height; ++Row)
		{
			Mask[Row * Width] = 0;
			Mask[Row * Width + Width - 1] = 0;
		}
	}

	// ── 텍스처 생성 ──
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0) return false;

	// 빈 칸도 RGB는 같게 두고 알파만 0 — 바이리니어 경계에 어두운 테두리가 생기지 않게
	const FColor Filled = pathColor.ToFColor(true);
	const FColor Empty(Filled.R, Filled.G, Filled.B, 0);

	// 텍스처는 안쪽 채움만 — 가장자리 선은 BuildContours가 만든 벡터 선으로 따로 그린다 (텍셀 계단 없이 매끈하게)
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	FColor* Dest = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
	for (int32 i = 0; i < Mask.Num(); ++i)
	{
		Dest[i] = Mask[i] ? Filled : Empty;
	}
	Mip.BulkData.Unlock();

	BuildContours(Mask, Width, Height, MinY, MaxX, Texel);

	Texture->SRGB = true;
	Texture->Filter = TF_Bilinear;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	Texture->NeverStream = true;
	Texture->UpdateResource();

	maskTexture = Texture;
	maskWorldMin = FVector2D(MinX, MinY);
	maskWorldMax = FVector2D(MaxX, MaxY);
	maskTexelSize = Texel;
	maskWidth = Width;
	maskHeight = Height;

	maskBrush = FSlateBrush();
	maskBrush.SetResourceObject(Texture);
	maskBrush.ImageSize = FVector2D(Width, Height);
	maskBrush.DrawAs = ESlateBrushDrawType::Image;
	maskBrush.Tiling = ESlateBrushTileType::NoTile;

	UE_LOG(LogTemp, Log, TEXT("[UC_MinimapWidget] 길 텍스처 생성 — %dx%d, 텍셀 %.1fcm, 폴리곤 %d개, 길 데이터 %s"),
		Width, Height, Texel, PolyCount, bUseRoadData ? *levelData->GetName() : TEXT("없음(내비메시 사용)"));
	return true;
}

void UC_MinimapWidget::BuildContours(const TArray<uint8>& Mask, int32 Width, int32 Height, double MinY, double MaxX, float Texel)
{
	contours.Reset();
	if (!bDrawPathEdge || Width < 3 || Height < 3) return;

	// ── 1) 마스크를 흐려서 매끈한 스칼라장으로 (3칸 박스 블러 2회) — 텍셀 계단을 없앤다 ──
	const int32 Num = Width * Height;
	TArray<float> Field;
	TArray<float> Temp;
	Field.SetNumUninitialized(Num);
	Temp.SetNumUninitialized(Num);
	for (int32 i = 0; i < Num; ++i) Field[i] = Mask[i] ? 1.f : 0.f;

	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (int32 Row = 0; Row < Height; ++Row)
		{
			for (int32 Col = 0; Col < Width; ++Col)
			{
				const int32 Index = Row * Width + Col;
				const float Left = Col > 0 ? Field[Index - 1] : 0.f;
				const float Right = Col < Width - 1 ? Field[Index + 1] : 0.f;
				Temp[Index] = (Left + Field[Index] + Right) / 3.f;
			}
		}
		for (int32 Row = 0; Row < Height; ++Row)
		{
			for (int32 Col = 0; Col < Width; ++Col)
			{
				const int32 Index = Row * Width + Col;
				const float Up = Row > 0 ? Temp[Index - Width] : 0.f;
				const float Down = Row < Height - 1 ? Temp[Index + Width] : 0.f;
				Field[Index] = (Up + Temp[Index] + Down) / 3.f;
			}
		}
	}

	// ── 2) 마칭 스퀘어 — 0.5 등치선을 선분으로. 선분의 양 끝은 "격자 변"의 ID로 식별해 나중에 잇는다 ──
	// 변 ID: 가로 변 (c,r)-(c+1,r) = 2*(r*W+c), 세로 변 (c,r)-(c,r+1) = 2*(r*W+c)+1
	const float Iso = 0.5f;
	TArray<FIntPoint> Segments;                 // (변 ID A, 변 ID B)
	TMap<int32, FIntPoint> EdgeToSegments;      // 변 ID → 그 변을 지나는 선분 최대 2개

	auto AddSegment = [&Segments, &EdgeToSegments](int32 EdgeA, int32 EdgeB)
	{
		const int32 SegIndex = Segments.Add(FIntPoint(EdgeA, EdgeB));
		for (const int32 Edge : { EdgeA, EdgeB })
		{
			FIntPoint& Pair = EdgeToSegments.FindOrAdd(Edge, FIntPoint(INDEX_NONE, INDEX_NONE));
			if (Pair.X == INDEX_NONE) Pair.X = SegIndex;
			else if (Pair.Y == INDEX_NONE) Pair.Y = SegIndex;
		}
	};

	for (int32 Row = 0; Row < Height - 1; ++Row)
	{
		for (int32 Col = 0; Col < Width - 1; ++Col)
		{
			const int32 Index = Row * Width + Col;
			int32 Case = 0;
			if (Field[Index] >= Iso) Case |= 1;                 // 좌상
			if (Field[Index + 1] >= Iso) Case |= 2;             // 우상
			if (Field[Index + Width + 1] >= Iso) Case |= 4;     // 우하
			if (Field[Index + Width] >= Iso) Case |= 8;         // 좌하
			if (Case == 0 || Case == 15) continue;

			const int32 Top = 2 * Index;
			const int32 Bottom = 2 * (Index + Width);
			const int32 Left = 2 * Index + 1;
			const int32 Right = 2 * (Index + 1) + 1;

			switch (Case)
			{
			case 1: case 14: AddSegment(Left, Top); break;
			case 2: case 13: AddSegment(Top, Right); break;
			case 3: case 12: AddSegment(Left, Right); break;
			case 4: case 11: AddSegment(Right, Bottom); break;
			case 6: case 9:  AddSegment(Top, Bottom); break;
			case 7: case 8:  AddSegment(Left, Bottom); break;
			case 5:  AddSegment(Left, Top); AddSegment(Right, Bottom); break;
			case 10: AddSegment(Top, Right); AddSegment(Left, Bottom); break;
			default: break;
			}
		}
	}

	// 변 ID → 등치선이 그 변을 지나는 지점 (월드 XY)
	auto EdgePoint = [&Field, Width, MinY, MaxX, Texel, Iso](int32 EdgeId) -> FVector2D
	{
		const int32 Index = EdgeId / 2;
		const bool bVertical = (EdgeId % 2) != 0;
		const float A = Field[Index];
		const float B = Field[bVertical ? Index + Width : Index + 1];
		const float Alpha = FMath::Abs(B - A) > KINDA_SMALL_NUMBER ? FMath::Clamp((Iso - A) / (B - A), 0.f, 1.f) : 0.5f;

		// 스칼라장 값은 텍셀 중심에 있다 → 픽셀 좌표 = 인덱스 + 0.5
		double PixelCol = (Index % Width) + 0.5;
		double PixelRow = (Index / Width) + 0.5;
		if (bVertical) PixelRow += Alpha; else PixelCol += Alpha;

		// 픽셀 → 월드 (굽기와 같은 매핑: 열 = (Y - MinY)/Texel + 1, 행 = (MaxX - X)/Texel + 1)
		return FVector2D(MaxX - (PixelRow - 1.0) * Texel, MinY + (PixelCol - 1.0) * Texel);
	};

	// ── 3) 선분을 이어 폴리라인으로 ──
	TArray<bool> Visited;
	Visited.SetNumZeroed(Segments.Num());

	for (int32 Start = 0; Start < Segments.Num(); ++Start)
	{
		if (Visited[Start]) continue;

		TArray<FVector2D> Points;
		int32 Current = Start;
		int32 EntryEdge = Segments[Start].X;
		Points.Add(EdgePoint(EntryEdge));

		while (Current != INDEX_NONE && !Visited[Current])
		{
			Visited[Current] = true;
			const int32 ExitEdge = Segments[Current].X == EntryEdge ? Segments[Current].Y : Segments[Current].X;
			Points.Add(EdgePoint(ExitEdge));

			const FIntPoint* Pair = EdgeToSegments.Find(ExitEdge);
			const int32 Next = Pair ? (Pair->X == Current ? Pair->Y : Pair->X) : INDEX_NONE;
			Current = Next;
			EntryEdge = ExitEdge;
		}

		if (Points.Num() < 4) continue;

		// 너무 짧은 고리(나무 하나가 만든 구멍 등)는 선으로 그리지 않는다
		double Length = 0.0;
		for (int32 i = 1; i < Points.Num(); ++i) Length += FVector2D::Distance(Points[i - 1], Points[i]);
		if (Length < minContourLength) continue;

		// ── 4) 모서리 깎기(Chaikin) 1회 — 남은 꺾임을 둥글게 ──
		const bool bClosed = Points[0].Equals(Points.Last(), 0.01);
		TArray<FVector2D> Smoothed;
		Smoothed.Reserve(Points.Num() * 2 + 2);
		const int32 SegCount = Points.Num() - 1;
		if (!bClosed) Smoothed.Add(Points[0]);
		for (int32 i = 0; i < SegCount; ++i)
		{
			Smoothed.Add(Points[i] * 0.75 + Points[i + 1] * 0.25);
			Smoothed.Add(Points[i] * 0.25 + Points[i + 1] * 0.75);
		}
		if (bClosed) Smoothed.Add(FVector2D(Smoothed[0]));   // 고리 닫기
		else Smoothed.Add(Points.Last());

		contours.Add(MoveTemp(Smoothed));
	}
}

bool UC_MinimapWidget::WorldToMap(const FVector& WorldLocation, float LocalSize, float EdgePadding, FVector2f& OutLocal) const
{
	// 월드 +X = 위, +Y = 오른쪽
	const float Half = LocalSize * 0.5f;
	const float Scale = Half / FMath::Max(viewRadius, 1.f);

	OutLocal.X = Half + static_cast<float>(WorldLocation.Y - centerLocation.Y) * Scale;
	OutLocal.Y = Half - static_cast<float>(WorldLocation.X - centerLocation.X) * Scale;

	return OutLocal.X >= EdgePadding && OutLocal.X <= LocalSize - EdgePadding
		&& OutLocal.Y >= EdgePadding && OutLocal.Y <= LocalSize - EdgePadding;
}

void UC_MinimapWidget::PaintDot(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry,
	const FVector2f& LocalCenter, float DotSize, const FLinearColor& Color) const
{
	const FVector2f Size(DotSize, DotSize);
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(LocalCenter - Size * 0.5f)),
		&dotBrush, ESlateDrawEffect::None, Color);
}

int32 UC_MinimapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const FVector2f FullSize = FVector2f(AllottedGeometry.GetLocalSize());
	const float LocalSize = FMath::Min(FullSize.X, FullSize.Y);
	if (LocalSize <= MinimapInnerPadding * 2.f) return Layer;

	// 배경 + 테두리
	++Layer;
	FSlateDrawElement::MakeBox(
		OutDrawElements, Layer,
		AllottedGeometry.ToPaintGeometry(FVector2f(LocalSize, LocalSize), FSlateLayoutTransform()),
		&backgroundBrush, ESlateDrawEffect::None, backgroundColor);   // MakeBox는 브러시 TintColor를 곱해 주지 않는다 — 직접 넘겨야 함

	if (!bHasCenter) return Layer;

	// 길/공터 — 플레이어 주변만 UV로 잘라서 그린다
	if (bMaskBaked && maskTexture && maskWidth > 0 && maskHeight > 0)
	{
		const float InnerSize = LocalSize - MinimapInnerPadding * 2.f;
		const float InnerRadius = viewRadius * (InnerSize / LocalSize);   // 안쪽 사각형이 담는 월드 반경

		const float CenterU = (static_cast<float>(centerLocation.Y - maskWorldMin.Y) / maskTexelSize + 1.f) / maskWidth;
		const float CenterV = (static_cast<float>(maskWorldMax.X - centerLocation.X) / maskTexelSize + 1.f) / maskHeight;
		const float HalfU = InnerRadius / maskTexelSize / maskWidth;
		const float HalfV = InnerRadius / maskTexelSize / maskHeight;

		maskBrush.SetUVRegion(FBox2f(
			FVector2f(CenterU - HalfU, CenterV - HalfV),
			FVector2f(CenterU + HalfU, CenterV + HalfV)));

		++Layer;
		FSlateDrawElement::MakeBox(
			OutDrawElements, Layer,
			AllottedGeometry.ToPaintGeometry(FVector2f(InnerSize, InnerSize),
				FSlateLayoutTransform(FVector2f(MinimapInnerPadding, MinimapInnerPadding))),
			&maskBrush, ESlateDrawEffect::None, FLinearColor::White);
	}

	// 길 가장자리 선 — 화면 근처 구간만 골라 이어진 선으로 그린다. 미니맵 밖으로 나가는 부분은 클립으로 잘라낸다
	if (bMaskBaked && contours.Num() > 0)
	{
		++Layer;
		const float InnerSize = LocalSize - MinimapInnerPadding * 2.f;
		OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry.MakeChild(
			FVector2f(InnerSize, InnerSize), FSlateLayoutTransform(FVector2f(MinimapInnerPadding, MinimapInnerPadding)))));

		const FPaintGeometry LineGeometry = AllottedGeometry.ToPaintGeometry();
		const float Half = LocalSize * 0.5f;
		const float Scale = Half / FMath::Max(viewRadius, 1.f);
		const double NearRange = viewRadius * 1.15;

		TArray<FVector2f> Run;
		auto FlushRun = [&Run, &OutDrawElements, &LineGeometry, Layer, this]()
		{
			if (Run.Num() >= 2)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, Layer, LineGeometry, Run,
					ESlateDrawEffect::None, pathEdgeColor, true, pathEdgeThickness);
			}
			Run.Reset();
		};

		for (const TArray<FVector2D>& Contour : contours)
		{
			bool bPrevNear = false;
			for (int32 i = 0; i < Contour.Num(); ++i)
			{
				const FVector2D& P = Contour[i];
				const bool bNear = FMath::Abs(P.X - centerLocation.X) <= NearRange && FMath::Abs(P.Y - centerLocation.Y) <= NearRange;
				const FVector2f Local(
					Half + static_cast<float>(P.Y - centerLocation.Y) * Scale,
					Half - static_cast<float>(P.X - centerLocation.X) * Scale);

				if (bNear)
				{
					// 화면 밖에서 들어오는 구간은 바로 앞 점부터 이어서 선이 가장자리에서 끊겨 보이지 않게
					if (!bPrevNear && i > 0)
					{
						const FVector2D& Prev = Contour[i - 1];
						Run.Add(FVector2f(
							Half + static_cast<float>(Prev.Y - centerLocation.Y) * Scale,
							Half - static_cast<float>(Prev.X - centerLocation.X) * Scale));
					}
					Run.Add(Local);
				}
				else if (bPrevNear)
				{
					Run.Add(Local);
					FlushRun();
				}
				bPrevNear = bNear;
			}
			FlushRun();
		}

		OutDrawElements.PopClip();
	}

	// 점 — 아래에서 위 순서: 포탈 → 상자 → 몬스터 → 플레이어
	++Layer;
	FVector2f MapPos;

	for (const TWeakObjectPtr<AActor>& Portal : trackedPortalsInactive)
	{
		if (Portal.IsValid() && WorldToMap(Portal->GetActorLocation(), LocalSize, portalDotSize * 0.5f, MapPos))
		{
			// 아직 열리지 않은 포탈은 흐리게
			PaintDot(OutDrawElements, Layer, AllottedGeometry, MapPos, portalDotSize, portalColor.CopyWithNewOpacity(portalColor.A * 0.35f));
		}
	}

	for (const TWeakObjectPtr<AActor>& Portal : trackedPortalsActive)
	{
		if (Portal.IsValid() && WorldToMap(Portal->GetActorLocation(), LocalSize, portalDotSize * 0.5f, MapPos))
		{
			PaintDot(OutDrawElements, Layer, AllottedGeometry, MapPos, portalDotSize, portalColor);
		}
	}

	for (const TWeakObjectPtr<AActor>& Chest : trackedChests)
	{
		if (Chest.IsValid() && !Chest->IsHidden() && WorldToMap(Chest->GetActorLocation(), LocalSize, chestDotSize * 0.5f, MapPos))
		{
			PaintDot(OutDrawElements, Layer, AllottedGeometry, MapPos, chestDotSize, chestColor);
		}
	}

	for (const TWeakObjectPtr<AActor>& Monster : trackedMonsters)
	{
		if (Monster.IsValid() && !Monster->IsHidden() && WorldToMap(Monster->GetActorLocation(), LocalSize, monsterDotSize * 0.5f, MapPos))
		{
			PaintDot(OutDrawElements, Layer, AllottedGeometry, MapPos, monsterDotSize, monsterColor);
		}
	}

	// 플레이어 — 항상 중심. 카메라가 보는 방향으로 짧은 선
	++Layer;
	const FVector2f MapCenter(LocalSize * 0.5f, LocalSize * 0.5f);

	const float YawRad = FMath::DegreesToRadians(viewYaw);
	const FVector2f ViewDir(FMath::Sin(YawRad), -FMath::Cos(YawRad));
	TArray<FVector2f> ViewLine;
	ViewLine.Add(MapCenter);
	ViewLine.Add(MapCenter + ViewDir * (playerDotSize * 1.4f));
	FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), ViewLine,
		ESlateDrawEffect::None, playerColor, true, 2.f);

	PaintDot(OutDrawElements, Layer, AllottedGeometry, MapCenter, playerDotSize, playerColor);

	return Layer;
}
