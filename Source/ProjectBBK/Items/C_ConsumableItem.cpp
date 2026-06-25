#include "C_ConsumableItem.h"
#include "ItemData.h"
#include "Engine/DataTable.h"

void AC_ConsumableItem::InitItem(FName InItemID)
{
	Super::InitItem(InItemID);

	if (!consumableDataTable) return;

	FConsumableItemData* Data = consumableDataTable->FindRow<FConsumableItemData>(InItemID, TEXT("ConsumableItem"));
	if (!Data) return;

	cachedItemName = Data->itemName;
	ApplyWorldMesh(Data->worldMesh);
}
