#include "C_TreasureChest.h"
#include "ItemData.h"
#include "C_ConsumableItem.h"
#include "C_EquipmentItem.h"
#include "C_MoneyItem.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AC_TreasureChest::AC_TreasureChest()
{
	// AC_BaseItem::OnItemBeginOverlap이 이 텍스트를 WBP_Interaction(interactionWidgetClass)에 그대로 넘긴다.
	cachedItemName = FText::FromString(TEXT("Open Box"));
}

void AC_TreasureChest::OnInteract(AC_BasePlayerCharactor* Player)
{
	if (!Player) return;

	if (openSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, openSound, GetActorLocation());
	}

	RollAndSpawnLoot();
	Destroy();
}

void AC_TreasureChest::RollAndSpawnLoot()
{
	// 1) 장비 필수 상자면 가장 싼 장비 1개를 먼저 확정 (음수 리스크 최소화를 위해 랜덤 전체 풀이 아닌 최저 value로 고정)
	FTreasureLootRow GuaranteedEquipment;
	const bool bHasGuaranteedEquipment = bIncludesEquipment && PickCheapestEquipment(GuaranteedEquipment);
	const int32 EquipmentValue = bHasGuaranteedEquipment ? GuaranteedEquipment.value : 0;

	// 장비 선차감 자체가 예산을 넘는 경우만 허용되는 예외 — 나머지 배정은 이 이후 예산 안에서만 이루어진다.
	const int32 BudgetAfterEquipment = FMath::Max(0, chestValue - EquipmentValue);

	// 2) 돈 몫은 chestValue 전체 기준 1~50% 랜덤
	const int32 Percent = FMath::RandRange(moneyPercentMin, moneyPercentMax);
	int32 MoneyValue = FMath::RoundToInt(chestValue * Percent / 100.0f);

	// 3) 장비+돈이 예산을 넘으면 돈을 줄여서 맞춘다
	MoneyValue = FMath::Clamp(MoneyValue, 0, BudgetAfterEquipment);

	// 4) 아이템 추첨용 남은 value
	int32 DrawBudget = BudgetAfterEquipment - MoneyValue;

	TArray<FTreasureLootRow> Pool;
	CollectLootPool(Pool);

	TArray<FName> DrawnConsumables;
	TArray<FName> DrawnEquipment;

	if (Pool.Num() > 0 && DrawBudget > 0)
	{
		int32 MinPoolValue = TNumericLimits<int32>::Max();
		for (const FTreasureLootRow& Row : Pool)
		{
			MinPoolValue = FMath::Min(MinPoolValue, Row.value);
		}

		// 반복 추첨이 예산 안에 못 들어가는 행만 계속 뽑는 최악의 경우를 대비한 안전 상한
		constexpr int32 MaxDrawAttempts = 500;
		int32 Attempts = 0;
		while (DrawBudget >= MinPoolValue && Attempts < MaxDrawAttempts)
		{
			++Attempts;

			const FTreasureLootRow& Row = Pool[FMath::RandRange(0, Pool.Num() - 1)];
			if (Row.value <= 0 || Row.value > DrawBudget)
			{
				continue;
			}

			DrawBudget -= Row.value;
			if (Row.bIsEquipment)
			{
				DrawnEquipment.Add(Row.itemID);
			}
			else
			{
				DrawnConsumables.Add(Row.itemID);
			}
		}
	}

	// 5) 다 못 채운 자투리 value는 돈으로 환산
	MoneyValue += DrawBudget;

	// 6) 스폰
	const FVector Origin = GetActorLocation();

	if (bHasGuaranteedEquipment)
	{
		SpawnEquipment(GuaranteedEquipment.itemID, FindGroundSpawnPoint(Origin, lootScatterRadius));
	}
	for (const FName& ItemID : DrawnEquipment)
	{
		SpawnEquipment(ItemID, FindGroundSpawnPoint(Origin, lootScatterRadius));
	}
	for (const FName& ItemID : DrawnConsumables)
	{
		SpawnConsumable(ItemID, FindGroundSpawnPoint(Origin, lootScatterRadius));
	}

	if (MoneyValue > 0)
	{
		const int32 BaseGold = MoneyValue * goldPerMoneyValue;
		const int32 GoldAmount = FMath::Max(0, FMath::RandRange(BaseGold - goldVariance, BaseGold + goldVariance));
		SpawnMoney(GoldAmount, FindGroundSpawnPoint(Origin, lootScatterRadius));
	}
}

bool AC_TreasureChest::PickCheapestEquipment(FTreasureLootRow& OutRow) const
{
	if (!equipmentDataTable)
	{
		return false;
	}

	TArray<FTreasureLootRow> Candidates;
	int32 MinValue = TNumericLimits<int32>::Max();

	for (const TPair<FName, uint8*>& Pair : equipmentDataTable->GetRowMap())
	{
		const FEquipmentItemData* Row = reinterpret_cast<const FEquipmentItemData*>(Pair.Value);
		if (!Row || Row->value <= 0)
		{
			continue;
		}

		if (Row->value < MinValue)
		{
			MinValue = Row->value;
			Candidates.Reset();
			Candidates.Add(FTreasureLootRow(Pair.Key, Row->value, true));
		}
		else if (Row->value == MinValue)
		{
			Candidates.Add(FTreasureLootRow(Pair.Key, Row->value, true));
		}
	}

	if (Candidates.Num() == 0)
	{
		return false;
	}

	OutRow = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	return true;
}

void AC_TreasureChest::CollectLootPool(TArray<FTreasureLootRow>& OutPool) const
{
	if (consumableDataTable)
	{
		for (const TPair<FName, uint8*>& Pair : consumableDataTable->GetRowMap())
		{
			const FConsumableItemData* Row = reinterpret_cast<const FConsumableItemData*>(Pair.Value);
			if (Row && Row->value > 0)
			{
				OutPool.Add(FTreasureLootRow(Pair.Key, Row->value, false));
			}
		}
	}

	if (bIncludesEquipment && equipmentDataTable)
	{
		for (const TPair<FName, uint8*>& Pair : equipmentDataTable->GetRowMap())
		{
			const FEquipmentItemData* Row = reinterpret_cast<const FEquipmentItemData*>(Pair.Value);
			if (Row && Row->value > 0)
			{
				OutPool.Add(FTreasureLootRow(Pair.Key, Row->value, true));
			}
		}
	}
}

FVector AC_TreasureChest::FindGroundSpawnPoint(const FVector& Center, float Radius) const
{
	const float Angle = FMath::FRand() * 2.f * PI;
	const float Distance = FMath::FRandRange(0.f, Radius);
	const FVector Candidate = Center + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 0.f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// 흩뿌린 위치 기준으로 바닥 탐색 (단차 대비 여유를 넉넉히 둠)
	FHitResult Hit;
	const FVector TraceStart = Candidate + FVector(0.f, 0.f, 500.f);
	const FVector TraceEnd = Candidate - FVector(0.f, 0.f, 1000.f);
	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		return Hit.ImpactPoint;
	}

	// 흩뿌린 위치에서 바닥을 못 찾은 경우(낭떠러지, 구멍 등) — 상자가 서 있던 자리 바로 아래를 재탐색.
	// 상자 자신은 반드시 바닥 위에 있었으므로 여기서는 항상 바닥을 찾는다. Candidate를 그대로 쓰면
	// 바닥으로 내려지지 않은 상자 높이 그대로 아이템이 공중에 뜬 채로 스폰되는 버그가 됨.
	FHitResult FallbackHit;
	const FVector FallbackStart = Center + FVector(0.f, 0.f, 500.f);
	const FVector FallbackEnd = Center - FVector(0.f, 0.f, 1000.f);
	if (GetWorld()->LineTraceSingleByChannel(FallbackHit, FallbackStart, FallbackEnd, ECC_Visibility, Params))
	{
		return FallbackHit.ImpactPoint;
	}

	return Center;
}

void AC_TreasureChest::SpawnConsumable(FName InItemID, const FVector& Location)
{
	if (!consumableItemClass || InItemID.IsNone())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AC_ConsumableItem* Spawned = GetWorld()->SpawnActor<AC_ConsumableItem>(consumableItemClass, Location, FRotator::ZeroRotator, Params))
	{
		Spawned->InitItem(InItemID);
		// 상자 위치에 플레이어가 이미 겹쳐 있을 수 있으므로(BeginOverlap이 델리게이트 바인딩 전에 지나갈 수 있음) 즉시 재확인
		Spawned->RefreshOverlapState();
	}
}

void AC_TreasureChest::SpawnEquipment(FName InItemID, const FVector& Location)
{
	if (!equipmentItemClass || InItemID.IsNone())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AC_EquipmentItem* Spawned = GetWorld()->SpawnActor<AC_EquipmentItem>(equipmentItemClass, Location, FRotator::ZeroRotator, Params))
	{
		Spawned->InitItem(InItemID);
		Spawned->RefreshOverlapState();
	}
}

void AC_TreasureChest::SpawnMoney(int32 InGoldAmount, const FVector& Location)
{
	if (!moneyItemClass || InGoldAmount <= 0)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AC_MoneyItem* Spawned = GetWorld()->SpawnActor<AC_MoneyItem>(moneyItemClass, Location, FRotator::ZeroRotator, Params))
	{
		Spawned->InitMoney(InGoldAmount);
		// InitMoney가 cachedItemName(골드 수치 텍스트)을 갱신하므로, 스폰 시점에 이미 걸려 있던
		// 오버랩 텍스트(BeginPlay 시점의 기본값 "0 gold")를 여기서 올바른 값으로 다시 반영한다.
		Spawned->RefreshOverlapState();
	}
}
