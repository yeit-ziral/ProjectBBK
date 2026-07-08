// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MerchantData.generated.h"

/**
 * 상인이 파는 물건 한 줄. DataTable(DT_MerchantStock) Row 구조체.
 * Row Name은 파는 아이템의 itemID(소비/장비 DT의 Row Name)와 동일하게 맞춘다.
 *
 * 가격은 아이템 데이터가 아니라 "상인 쪽"에 둔다 — 아이템 데이터(FBaseItemData 등)는
 * 아이템 담당 파트라 건드리지 않기 위함. 추후 아이템에 가치(value) 필드가 생기면
 * AC_MerchantNPC::GetBuyPrice / GetSellPrice 두 함수만 "아이템 값 우선 조회"로 바꾸면 됨.
 */
USTRUCT(BlueprintType)
struct FMerchantStockRow : public FTableRowBase
{
	GENERATED_BODY()

	// 파는 아이템 ID (소비/장비 DataTable의 Row Name과 일치)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant")
	FName itemID = NAME_None;

	// 구매가 (플레이어가 사면 이만큼 지불)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant", meta = (ClampMin = "0"))
	int32 buyPrice = 0;

	// 재고 수량. -1 = 무한 판매. 0 이상이면 그만큼만 판매 가능.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Merchant", meta = (ClampMin = "-1"))
	int32 stock = -1;

	FMerchantStockRow() {}
};

/**
 * 상점 UI에 넘기기 위한 런타임용 재고 항목 (Row Name = itemID 병합).
 */
USTRUCT(BlueprintType)
struct FMerchantStockEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Merchant")
	FName itemID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Merchant")
	int32 buyPrice = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Merchant")
	int32 stock = -1;

	FMerchantStockEntry() {}
	FMerchantStockEntry(FName InID, int32 InPrice, int32 InStock)
		: itemID(InID), buyPrice(InPrice), stock(InStock) {}
};
