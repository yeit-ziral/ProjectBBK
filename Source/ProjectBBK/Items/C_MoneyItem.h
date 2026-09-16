#pragma once

#include "CoreMinimal.h"
#include "C_BaseItem.h"
#include "C_MoneyItem.generated.h"

USTRUCT(BlueprintType)
struct FMoneyMeshTier
{
	GENERATED_BODY()

	// moneyAmount가 이 값 이상일 때 이 mesh를 사용 (여러 tier 중 조건을 만족하는 가장 큰 minAmount가 선택됨)
	UPROPERTY(EditAnywhere, Category = "Item|Money")
	int32 minAmount = 0;

	UPROPERTY(EditAnywhere, Category = "Item|Money")
	UStaticMesh* mesh = nullptr;
};

UCLASS()
class PROJECTBBK_API AC_MoneyItem : public AC_BaseItem
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void OnInteract(AC_BasePlayerCharactor* Player) override;

	UFUNCTION(BlueprintCallable, Category = "Item|Money")
	void InitMoney(int32 InAmount);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Money")
	int32 moneyAmount = 0;

	// BP_MoneyItem에서 moneyAmount 구간별 mesh를 등록 (예: 0/10/50/100)
	UPROPERTY(EditDefaultsOnly, Category = "Item|Money")
	TArray<FMoneyMeshTier> moneyMeshTiers;

private:
	void ApplyMeshForAmount();
};
