#pragma once

#include "CoreMinimal.h"
#include "C_BaseItem.h"
#include "C_MoneyItem.generated.h"

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
};
