#pragma once

#include "CoreMinimal.h"
#include "C_BaseItem.h"
#include "ItemData.h"
#include "C_ConsumableItem.generated.h"

class UDataTable;

UCLASS()
class PROJECTBBK_API AC_ConsumableItem : public AC_BaseItem
{
	GENERATED_BODY()

public:
	virtual void InitItem(FName InItemID) override;

protected:
	virtual void OnInteract(AC_BasePlayerCharactor* Player) override;

	UPROPERTY(EditDefaultsOnly, Category = "Item|Data")
	UDataTable* consumableDataTable;

private:
	FConsumableItemData cachedData;
	bool bDataLoaded = false;
};
