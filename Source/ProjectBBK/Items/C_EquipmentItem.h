#pragma once

#include "CoreMinimal.h"
#include "C_BaseItem.h"
#include "ItemData.h"
#include "ActiveGameplayEffectHandle.h"
#include "C_EquipmentItem.generated.h"

class UDataTable;

UCLASS()
class PROJECTBBK_API AC_EquipmentItem : public AC_BaseItem
{
	GENERATED_BODY()

public:
	virtual void InitItem(FName InItemID) override;

protected:
	virtual void OnInteract(AC_BasePlayerCharactor* Player) override;

	UPROPERTY(EditDefaultsOnly, Category = "Item|Data")
	UDataTable* equipmentDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Item|GAS")
	TSubclassOf<UGameplayEffect> GE_EquipBonus;

private:
	FEquipmentItemData cachedData;
	bool bDataLoaded = false;
};
