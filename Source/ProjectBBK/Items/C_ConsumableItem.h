#pragma once

#include "CoreMinimal.h"
#include "C_BaseItem.h"
#include "C_ConsumableItem.generated.h"

class UDataTable;

UCLASS()
class PROJECTBBK_API AC_ConsumableItem : public AC_BaseItem
{
	GENERATED_BODY()

public:
	virtual void InitItem(FName InItemID) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Item|Data")
	UDataTable* consumableDataTable;
};
